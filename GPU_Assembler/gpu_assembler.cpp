#include <iostream> // cout, cerr, cin
#include <vector>   // vector<>
#include <map>      // map<>
#include <stdio.h>  // FILE
#include <stdint.h> // uint64_t
#include <stdlib.h> // strtol(), strtoul(), strtof(), exit()
#include <errno.h>  // errno, ERANGE, EINVAL
#include <unistd.h> // getopt()

using namespace std;





// Structs, global Vars and enums
struct relocation {
    string label;                   // branch label
    int pc;                         // branch instruction address
};

struct context {
    const char *stream;             // ?
    map<string, int> labels;        // maps relocation label to pc address
    int pc;                         // program counter
    vector<relocation> relocations; // vector of all branch relocations
};

struct QPUreg {
    enum {A, B, ACCUM, SMALL} file; // QPU register could be an ra register,
                                    // rb register, r register or small immed.
    uint32_t num;                   // number of register or small immed. value
                                    // TODO: If QPUreg num only used in alu
                                    // and lu small imm instruction, and not
                                    // in branch, or load imm 32 instruction
                                    // it is sufficient to make num uint8_t
                                    // But should I? Error detection maybe
                                    // better when 32 bit, because of possible
                                    // overflow acceptance
};

enum token_t {
    END=-1,         // '#', end reading line when this token is encountered
                    //      in qasm code it signifies start of comment
    WORD,           // number or instruction
    DOT,            // '.'
    COMMA,          // ','
    SEMI,           // ';'
    COLON,          // ':'
};

enum op_t {
    INVALID,
    ALU,
    BRANCH,
    LDI,
    SEMA
};

static string add_ops[] = {
    "nop", "fadd", "fsub", "fmin", "fmax", "fminabs", "fmaxabs",
    "ftoi", "itof", "XXX", "XXX", "XXX", "add", "sub", "shr",
    "asr", "ror", "shl", "min", "max", "and", "or", "xor", "not",
    "clz", "XXX", "XXX", "XXX", "XXX", "XXX", "v8adds", "v8subs" };

static string mul_ops[] = {
    "nop", "fmul", "mul24", "v8muld", "v8min", "v8max", "v8adds",
    "v8subs" };





// Functions
token_t nextToken(const char *stream, string& token_string, const char **stream_ptr);

static uint8_t addOpCode(const string& token_string);

static uint8_t mulOpCode(const string& token_string);

uint64_t assembleALUInstr(context& ctx, string token_string);

bool parseALUPipe(const char *stream, QPUreg& dest, QPUreg& r1, QPUreg& r2, uint8_t& sig,
                  uint8_t& unpack, uint8_t& pm, uint8_t& pack, const char **stream_ptr);

bool parseRegister(const string& token_string, QPUreg& reg);

uint32_t parseImmediate(const string& token_string);

uint8_t setALUMux(const QPUreg& reg);

uint64_t assembleBRANCHInstr(context& ctx, string token_string);

uint8_t parseBranchCondition(const string& token_string);

uint64_t assembleLDIInstr(context& ctx, string token_string);

uint64_t assembleSEMAInstr(context& ctx, string token_string);





/* main, "Usage: " << argv[0] << " in.asm [-o out.bin]"
 * Assembles a file writen in assembly language according to the VideCore IV 3D
 * Architecture Reference Guide (VideCoreIV-AG100-R, Issue September 16, 2013)
 * which is intended for the GPU.
 * This Assembler is not functional complete, but more importantly does not
 * throw errors on most architecture instruction violations in the assembly
 * code! Restrictions are detailed in the Architecture Reference Guide (ARG).

 */
int main(int argc, char* argv[])
{
    char *fname_out = (char *) "out.bin";    // name of output file
    FILE *file_out;                 // output file descriptor
    FILE *file_in;                  // input file descriptor
    vector<uint64_t> instructions;  // buffer to save the binary instructions
    char line[128];                 // buffer for single asm lines
    string token_string;            // string to save the next token
    struct context ctx;             // ?
    token_t tok;                    // var to save next token type
    int line_no;                    // saves current line number for reloc
    int c;                          // temporary variable for getopt


    file_in = fopen(argv[1], "r");
    if (!file_in) {
        cerr << "Unable to open input asm file " << endl;
        cerr << "Usage: " << argv[0] << " in.asm [-o out.bin]" << endl;
        return -6;
    }


    while ((c = getopt(argc, argv, "o:")) != -1) {
        switch (c) {
            case 'o':
                fname_out = optarg;
                break;
        }
    }

    if (fname_out == "out.bin") {
        cout << "No seperate output file recognized. Binary output saved to '"\
            << fname_out << "'" << endl;
    }


    // Start of Core Assembler functionality

    ctx.pc = 0;
    line_no = 0;

    // Read in .qasm input and process each line it
    while(fgets(line, sizeof(line), file_in)) {
        line_no++;
        ctx.stream = line;

        cout << "Processing line: " << line_no << " ..." << endl;


        // Assign 'tok' the next token
        tok = nextToken(ctx.stream, token_string, &ctx.stream);


        // When tok equals END, read next line as it signifies start of
        // comment. The cases in which tok equals DOT, COMMA, SEMI, COLON are
        // unhandled as they don't make sense at beginning of instructions and
        // currently simply result in the next line being read.
        if (tok == END)
            continue;



        if (tok == WORD) {
            // Read ahead to see if the next token is a colon in which case
            // this current token is a label for relocation/branch.
            const char *discard_ptr;    // to not advance ctx.stream
            string discard_string;      // to not overwrite token_string
            if (nextToken(ctx.stream, discard_string, &discard_ptr) == COLON) {
                // Current token is a label for relocation. Save it and
                // continue with next line
                ctx.labels[token_string] = ctx.pc;
                continue;
            }



            // If token_string represents an op-code, set upcoming OP to ALU.
            // Otherwise, if token_string represents "LDI", "BRANCH" or "SEMA"
            // set op_type accordingly
            op_t op_type = INVALID;

            if (addOpCode(token_string) != 0xFF || mulOpCode(token_string) != 0xFF)
                op_type = ALU;

            if (token_string == "ldi")
                op_type = LDI;

            if (token_string == "bra" || token_string == "brr")
                op_type = BRANCH;

            if (token_string == "sema")
                op_type = SEMA;


            if (op_type == INVALID) {
                cerr << "Unable to assemble line " << line_no << " : " << line << endl;
                cerr << " ... invalid opcode" << endl;
                return -2;
            }



            // Now assembler instruction to machine code according to the
            // type of the OP and token string. Then save the binary
            // instruction to the instructions buffer and increase the program
            // counter
            uint64_t instruction = 0x0;
            switch (op_type) {
                case ALU:
                    instruction = assembleALUInstr(ctx, token_string);
                    break;
                case BRANCH:
                    instruction = assembleBRANCHInstr(ctx, token_string);
                    break;
                case LDI:
                    instruction = assembleLDIInstr(ctx, token_string);
                    break;
                case SEMA:
                    instruction = assembleSEMAInstr(ctx, token_string);
                    break;
            }

            if (instruction == (uint64_t) -1) {
                cerr << "Error on line " << line_no << " : " << line << endl;
                return -3;
            }

            instructions.push_back(instruction);
            ctx.pc += 8;    // increased in bytes to next instr (64 bit/8 byte)
        }
    }



    // Process the relocations that branch forward in the code and couldn't
    // therefore be processed yet
    ctx.labels["BRANCHBACK"] = 0x0;
    for (int i=0; i < ctx.relocations.size(); i++) {
        uint64_t instruction;
        uint32_t immediate;
        uint8_t rel;
        relocation& r = ctx.relocations[i];

        if (ctx.labels.count(r.label) < 1) {
            cerr << "ERROR. Undefined label: " << r.label << endl;
            return -4;
        }

        // Load the instruction whose immediate needs to be relocated.
        // Chosen instruction r.pc / 8 because a single instruction needs 8
        // byte in memory
        instruction = instructions[r.pc / 8];


        // read the 'rel' instruction field from the current instruction to
        // determine if branch is relative or not
        rel = (instruction >> 51) & 1;


        // If branch is relative, the branch_target is relative to the link address
        // (current pc + 4*8 as branch happens 4 instructions after call and one
        // instructions equals 8 bytes)
        if (rel)
            immediate = ctx.labels[r.label] - (r.pc + 4*8);
        else
            immediate = ctx.labels[r.label];


        if (r.label == "BRANCHBACK") {
            // Branchback absolute branch from start of code
            immediate = 0x0;
            if (rel) {
                cerr << "ERROR. Branchback not with absolute branch 'bra'" << endl;
                return -5;
            }
        }

        cout << "Processing relocation at " << r.pc << " : " << r.label
             << " : " << immediate << endl;


        // Zero out previous immediate (representing branch target location)
        // and replace with newly calculated immediate from the relocations.
        instruction &= ((uint64_t)0xffffffff << 32);
        instruction |= ((uint32_t)immediate << 0);
        instructions[r.pc / 8] = instruction;
    }


    // End of Core Assembler funcionality



    file_out = fopen(fname_out, "w");
    if (!file_out) {
        cerr << "Unable to open output file <output.bin>" << endl;
        cerr << "Usage: " << argv[0] << " in.asm [-o out.bin]" << endl;
        return -2;
    }

    for (int i=0; i < instructions.size(); i++) {
        fwrite(&instructions[i], sizeof(uint64_t), 1, file_out);
    }

    fclose(file_out);
    fclose(file_in);

    cout << "Done.  Num instructions: " << instructions.size() << ", "
         << instructions.size() * 8 << " bytes." << endl;
}





/* Identify the next characters in 'stream' as special token (END, DOT, COMMA,
 * SEMI, COLON) or digit (WORD) or instruction (WORD). Then save the next
 * tokenized characters in 'token_string', advance the 'stream_ptr' to after
 * the tokenized string and return the token type.
 *
 *  *stream: stream of characters
 *  token_string: string into which the tokenized character is saved
 *  stream_ptr: address to which the pointer stream is advanced after
 *      tokenizing the next word
 */
token_t nextToken(const char *stream, string& token_string, const char **stream_ptr)
{
    char buffer[128];
    int i = 0;

    *stream_ptr = stream;


    // If stream not available return 'END' token signaling end of instructions
    if (!stream || !*stream)
        return END;


    // Traverse whitespace
    while (*stream == ' ' || *stream == '\t') {
        stream++;
    }


    // If file ends return 'END' token signaling end of instructions
    if (*stream == '\0' || *stream == '\n')
        return END;


    // If stream is at number, save the number to 'token_string', advance the
    // 'stream_ptr' to after the number and return token 'WORD'
    if (isdigit(*stream))
    {
        // Traverse until we don't find a hex digit, x (for hex) or .
        // meanwhile save the traversed number in 'buffer'
        while (isxdigit(*stream) || *stream == '.' || *stream == 'x') {
            buffer[i++] = *stream++;
            if (*stream == 0 || i > sizeof(buffer) - 1)
                break;
        }
        buffer[i++] = '\0';
        token_string = buffer;
        *stream_ptr = stream;

        return WORD;
    }


    // If stream contains a DOT, COMMA, SEMI, END or COLON return this after
    // advancing the 'stream_ptr'
    if (*stream == '.') {
        stream++;
        *stream_ptr = stream;
        return DOT;
    }
    if (*stream == ',') {
        stream++;
        *stream_ptr = stream;
        return COMMA;
    }
    if (*stream == ';') {
        stream++;
        *stream_ptr = stream;
        return SEMI;
    }
    if (*stream == '#') {
        stream++;
        *stream_ptr = stream;
        return END;
    }
    if (*stream == ':') {
        stream++;
        *stream_ptr = stream;
        return COLON;
    }


    // As the next token is neither a digit or a special token traverse the
    // next characters while they are no special tokens or whitespace.
    // Save the traversed characters in 'token_string', advance the
    // 'stream_ptr' and return WORD
    while (*stream != '.' && *stream != ',' && *stream != ';' && *stream != ':'
           && *stream != ' ' && *stream != '\t') {
        buffer[i++] = *stream++;
        if (*stream == 0 || *stream == '\n' || i > sizeof(buffer)-1)
            break;
    }

    buffer[i++] = '\0';
    token_string = buffer;
    *stream_ptr = stream;

    return WORD;
}





/* Compares token_string with all available add-Ops. If token_string
 * matches with an add-Op, the corresponding opcode is returned, otherwise 0xFF
 */
static uint8_t addOpCode(const string& token_string)
{
    for (int i=0; i < 32; i++) {
        if (token_string == add_ops[i])
            return i;
    }

    return 0xFF;
}





/* Compares token_string with all available mul-Ops. If token_string
 * matches with an mul-Op, the corresponding opcode is returned, otherwise 0xFF
 */
static uint8_t mulOpCode(const string& token_string)
{
    for (int i=0; i < 8; i++) {
        if (token_string == mul_ops[i])
            return i;
    }

    return 0xFF;
}





/* Reads current line from 'ctx' and assembles the add and mul pipe
 * instructions into a binary encoding of the instruction line which it then
 * returns. An ALU Instruction lines has complication restrictions which
 * are listed as comment within this function. Examples for valid ALU lines:
 *      add ra1, ra20, rb22; mul24 rb12, r0, rb22
 *      and ra34, ra12, 0x12; nop
 *      xor rb2, ra24, r0; v8max ra7, ra24, r1;
 */
uint64_t assembleALUInstr(context& ctx, string token_string)
{
    uint64_t instruction;   // var for binary encoding of asm instruction line


    // Variables necessary to assemble instruction. For detailed description
    // see ARG, p.26
    uint8_t sig;
    uint8_t unpack;
    uint8_t pm;
    uint8_t pack;
    uint8_t cond_add;
    uint8_t cond_mul;
    uint8_t sf;
    uint8_t ws;
    uint8_t waddr_add;
    uint8_t waddr_mul;
    uint8_t op_mul;
    uint8_t op_add;
    uint8_t raddr_a;
    uint8_t raddr_b;
    uint8_t add_a;
    uint8_t add_b;
    uint8_t mul_a;
    uint8_t mul_b;


    // Variables necessary for this function
    bool parse_mul;         // bool if mul instr should be parsed
    token_t tok;            // var to save next token type
    QPUreg add_dest, add_r1, add_r2;    // structs to save reg file and num of
    QPUreg mul_dest, mul_r1, mul_r2;    // the dest reg and both argument regs


    parse_mul = true;       // assume initially that mul instr has to be read



    // **********************************************
    // BEGINNING OF PARSING OF WHOLE INSTRUCTION LINE
    // **********************************************

    // Parse instruction and arguments for add pipeline
    op_add = addOpCode(token_string);
    if (op_add == 0xFF) {
        cerr << "FATAL (assert). Bad add opcode" << endl;
        return (uint64_t) -1;
    }

    if (!parseALUPipe(ctx.stream, add_dest, add_r1, add_r2, sig, unpack, pm, pack, &ctx.stream))
        return (uint64_t) -1;



    // Read and assert the next token to be a semicolon
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != SEMI) {
        cerr << "FATAL (assert). Missing semicolon" << endl;
        return (uint64_t) -1;
    }



    // Read and parse next instruction for mul pipeline, then decide whether
    // the mul pipeline consists only of nop and skip parsing if so. If not,
    // read the whole mul instruction and save the registers values
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    op_mul = mulOpCode(token_string);
    if (op_mul == 0xFF) {
        cerr << "FATAL (assert). Bad mul opcode" << endl;
        return (uint64_t) -1;
    }

    if (op_mul == 0) { // test if op_mul == nop
        // If the next token is a SEMI or END, the registers and arguments are
        // auto generated to a nop write (ra39, rb39) and same arguments as
        // the add pipe
        const char *discard_ptr;    // to not advance ctx.stream

        tok = nextToken(ctx.stream, token_string, &discard_ptr);
        if (tok == END || tok == SEMI) {
            mul_dest.num = 39;
            mul_dest.file = (add_dest.file==QPUreg::A) ? QPUreg::B : QPUreg::A;

            mul_r1.num = add_r1.num;
            mul_r1.file = add_r1.file;

            mul_r2.num = add_r2.num;
            mul_r2.file = add_r2.file;

            parse_mul = false;
        }
    }

    if (parse_mul) {
        // As the signaling bit, unpack, pm and pack values are all read from
        // the add pipeline instruction, we throw away any possible further
        // values from the mul pipeline
        uint8_t discard_int;
        if(!parseALUPipe(ctx.stream, mul_dest, mul_r1, mul_r2, discard_int,
                         discard_int, discard_int, discard_int, &ctx.stream))
            return (uint64_t) -1;
    }



    // Check the parsed QPUreg structs if they are allowed according to the
    // ARG restrictions which are summarized in the following:
    //
    // add rS, rX, rY; mul rT, rZ, rW;
    //
    // rS, rT: Generally allowed to be either A regfile or a B regfile.
    // Areg, Areg:     NOT allowed
    // Areg, Breg:     allowed, ws not set
    // Breg, Areg:     allowed, ws set
    // Breg, Breg:     NOT allowed
    //
    // rX, rZ: Generally allowed to be either A regfile or Accumulator
    // Areg, Areg:     allowed IF both are the same register
    // Areg, Accum:    allowed, no restrictions
    // Accum, Areg:    allowed, no restrictions
    // Accum, Accum:   allowed, no restrictions
    //
    // rY, rW: Generally allowed to be B regfile, Accumulator or Small Immed
    // Breg, Breg:     allowed IF both are the same register
    // Breg, Accum:    allowed, no restrictions
    // Breg, Small:    NOT allowed
    // Accum, Breg:    allowed, no restrictions
    // Accum, Accum:   allowed, no restrictions
    // Accum, Small:   allowed, no restrictions
    // Small, Breg:    NOT allowed
    // Small, Accum:   allowed, no restrictions
    // Small, Small:   allowed IF both are same Small Immediate

    if (add_dest.file == mul_dest.file) {
        cerr << "FATAL (assert). Add and Mul Pipe write to same register file" << endl;
        return (uint64_t) -1;
    }

    if ((add_r1.file == QPUreg::A) && (mul_r1.file == QPUreg::A)
        && (add_r1.num != mul_r1.num)) {
        cerr << "FATAL (assert). First read argument of add and mul pipeline "
             << "are not the same A regfile." << endl;
        return (uint64_t) -1;
    }

    if ((add_r2.file == QPUreg::B) && (mul_r2.file == QPUreg::B)
        && (add_r2.num != mul_r2.num)) {
        cerr << "FATAL (assert). Second read argument of add and mul pipeline "
             << "are not the same B regfile." << endl;
        return (uint64_t) -1;
    }
    if ((add_r2.file == QPUreg::SMALL) && (mul_r2.file == QPUreg::SMALL)
        && (add_r2.num != mul_r2.num)) {
        cerr << "FATAL (assert). Second read argument of add and mul pipeline "
             << "are not the same Small Immediate." << endl;
        return (uint64_t) -1;
    }
    if ((add_r2.file == QPUreg::B) && (mul_r2.file == QPUreg::SMALL) ||
        (add_r2.file == QPUreg::SMALL) && (mul_r2.file == QPUreg::B)) {
        cerr << "FATAL (assert). Second read argument of add and mul pipeline "
             << "are not allowed to be B regfile and also  Small Immediate" << endl;
        return (uint64_t) -1;
    }





    // *********************************************************************
    // BEGINNING OF ASSEMBLY OF THE BINARY INSTRUCTION ACCORDING TO ARG p.26
    // *********************************************************************

    // sig, if second read argument contains small immediate, the ALU
    // instruction becomes a 'alu small imm' instruction (see ARG p26) which
    // has to be signified by changing the sig value to 1101
    if (add_r2.file == QPUreg::SMALL || mul_r2.file == QPUreg::SMALL) {
        if (sig != 1) { // if sig not 'No signal'
            cerr << "FATAL (assert). Special ALU signaling bit set while also "
                 << "loading a small immediate" << endl;
            return (uint64_t) -1;
        }

        sig = 13;   // equals '1101' in binary
    }


    // unpack, TODO: Description. Already determined.
    unpack = unpack;


    // pm, TODO: Description. Already determined.
    pm = pm;


    // pack, TODO: Description. Already determined.
    pack = pack;


    // cond_add, TODO: Description
    cond_add = 1;   // add condition set to 'always'


    // cond_mul, TODO: Description
    cond_mul = 1;   // mul condition set to 'always'


    // sf, Bit that determines if N, Z, C conditions will be updated
    sf = 1;
    if ((op_add == 0) && (op_mul == 0)) {
      sf = 0;
    }


    // ws, determine if 'write_swap' occurs (if add pipe writes to B reg file
    // and mul pipe writes to A reg file)
    if (add_dest.file == QPUreg::A)
        ws = 0;   // false
    else
        ws = 1;   // true


    // waddr_add, number of the register that is written to by add ALU.
    waddr_add = add_dest.num;


    // waddr_mul, number of the register that is written to by mul ALU.
    waddr_mul = mul_dest.num;


    // op_mul, executed op in mul ALU. Already determined.
    op_mul = op_mul;


    // op_add, executed op in add ALU. Already determined.
    op_add = op_add;


    // raddr_a, saves the number of physical ra register, if one is addressed
    raddr_a = 0;
    if (add_r1.file == QPUreg::A)
        raddr_a = add_r1.num;
    else if (mul_r1.file == QPUreg::A)
        raddr_a = mul_r1.num;


    // raddr_b, saves the number of physical rb register, if one is addressed
    // or saves the value of the used Small Immediate
    raddr_b = 0;
    if (add_r2.file == QPUreg::B || add_r2.file == QPUreg::SMALL)
        raddr_b = add_r2.num;
    else if (mul_r2.file == QPUreg::B || mul_r2.file == QPUreg::SMALL)
        raddr_b = mul_r2.num;


    // add_a, saves if first read argument of add pipe is A regfile or Accum,
    // in which case it saves the number of the Accum.
    add_a = setALUMux(add_r1);


    // add_b, saves if second read argument of add pipe is B regfile, Small
    // Immediate or Accum, in which case it saves the number of the Accum.
    add_b = setALUMux(add_r2);


    // mul_a, saves if first read argument of mul pipe is A regfile or Accum,
    // in which case it saves the number of the Accum.
    mul_a = setALUMux(mul_r1);


    // mul_b, saves if second read argument of mul pipe is B regfile, Small
    // Immediate or Accum, in which case it saves the number of the Accum.
    mul_b = setALUMux(mul_r2);



    // Encoding of all single instruction fields into one 64-bit instruction
    instruction = 0x0;
    instruction |= ((uint64_t)sig << 60);
    instruction |= ((uint64_t)unpack << 57);
    instruction |= ((uint64_t)pm << 56);
    instruction |= ((uint64_t)pack << 52);
    instruction |= ((uint64_t)cond_add << 49);
    instruction |= ((uint64_t)cond_mul << 46);
    instruction |= ((uint64_t)sf << 45);
    instruction |= ((uint64_t)ws << 44);
    instruction |= ((uint64_t)waddr_add << 38);
    instruction |= ((uint64_t)waddr_mul << 32);
    instruction |= ((uint64_t)op_mul << 29);
    instruction |= ((uint64_t)op_add << 24);
    instruction |= ((uint64_t)raddr_a << 18);
    instruction |= ((uint64_t)raddr_b << 12);
    instruction |= ((uint64_t)add_a << 9);
    instruction |= ((uint64_t)add_b << 6);
    instruction |= ((uint64_t)mul_a << 3);
    instruction |= ((uint64_t)mul_b << 0);


    return instruction;
}





/* parseALUPipe reads stream beginning after the add/mul instruction ends.
 * It then reads along and determines any ALU signal, unpack, pm and pack bits,
 * the destination register, and both operation register values and saves them
 * to the accordingly passed QPUreg structs and sig value.
 * The function closes by advancing the stream_ptr to the end of the whole
 * pipeline (add or mul pipeline respectively) instruction.
 */
bool parseALUPipe(const char *stream, QPUreg& dest, QPUreg& r1, QPUreg& r2, uint8_t& sig,
                  uint8_t& unpack, uint8_t& pm, uint8_t& pack, const char **stream_ptr)
{
    token_t tok;
    string token_string;

    tok = nextToken(stream, token_string, &stream);


    // If next token equals 'DOT', the Op has a condition. This condition is
    // then determined and saved in the 'sig','unpack','pm' or 'pack' bits.
    // Otherwise those instruction fields will be set to standard values
    // For an overview of all conditions, see ARG, p.29
    sig = 1;        // sig set to 'no signal'
    unpack = 0;     // unpack set to 'no unpack'
    pm = 0;         // pm set to 'pack and unpack on regfile A'
    pack = 0;       // pack set to 'no pack'
    if (tok == DOT) {
        nextToken(stream, token_string, &stream);

        cout << "Parsing conditional: " << token_string << endl;

        if (token_string == "tmu" || token_string == "tmu0")
            sig = 10;       // ALU sig set to 'Load data from TMU0 to r4'
        else if (token_string == "tmu1")
            sig = 11;       // ALU sig set to 'Load data from TMU1 to r4'
        else if (token_string == "tend")
            sig = 3;        // ALU sig set to 'Program End (Thread End)'

        else if (token_string == "unpack32")
            unpack = 0;
        else if (token_string == "unpack16a")
            unpack = 1;
        else if (token_string == "unpack16b")
            unpack = 2;
        else if (token_string == "unpack8ddupe")
            unpack = 3;
        else if (token_string == "unpack8a")
            unpack = 4;
        else if (token_string == "unpack8b")
            unpack = 5;
        else if (token_string == "unpack8c")
            unpack = 6;
        else if (token_string == "unpack8d")
            unpack = 7;

        else if (token_string == "pack32")
            pack = 0;
        else if (token_string == "pack16a")
            pack = 1;
        else if (token_string == "pack16b")
            pack = 2;
        else if (token_string == "pack8ddupe")
            pack = 3;
        else if (token_string == "pack8a")
            pack = 4;
        else if (token_string == "pack8b")
            pack = 5;
        else if (token_string == "pack8c")
            pack = 6;
        else if (token_string == "pack8d")
            pack = 7;
        else if (token_string == "pack32clamp")
            pack = 8;
        else if (token_string == "pack16aclamp")
            pack = 9;
        else if (token_string == "pack16bclamp")
            pack = 10;
        else if (token_string == "pack8ddupeclamp")
            pack = 11;
        else if (token_string == "pack8aclamp")
            pack = 12;
        else if (token_string == "pack8bclamp")
            pack = 13;
        else if (token_string == "pack8cclamp")
            pack = 14;
        else if (token_string == "pack8dclamp")
            pack = 15;

        else if (token_string == "PMunpack32") {
            pm = 1;
            unpack = 0;
        } else if (token_string == "PMunpack16a") {
            pm = 1;
            unpack = 1;
        } else if (token_string == "PMunpack16b") {
            pm = 1;
            unpack = 2;
        } else if (token_string == "PMunpack8ddupe") {
            pm = 1;
            unpack = 3;
        } else if (token_string == "PMunpack8a") {
            pm = 1;
            unpack = 4;
        } else if (token_string == "PMunpack8b") {
            pm = 1;
            unpack = 5;
        } else if (token_string == "PMunpack8c") {
            pm = 1;
            unpack = 6;
        } else if (token_string == "PMunpack8d") {
            pm = 1;
            unpack = 7;
        }

        else if (token_string == "PMpack32") {
            pm = 1;
            pack = 0;
        } else if (token_string == "PMpack8ddupe") {
            pm = 1;
            pack = 3;
        } else if (token_string == "PMpack8a") {
            pm = 1;
            pack = 4;
        }

        else {
            cerr << "ERROR. Could not match conditional" << endl;
            return false;
        }

        tok = nextToken(stream, token_string, &stream);
    }


    // Upcoming Tokens should represent WORD (for dest), COMMA, WORD (for r1),
    // COMMA, WORD (for r2)
    // Small immediates are allowed in r2 (and currently only in r2)
    if (tok != WORD)
        return false;
    if (!parseRegister(token_string, dest)) {
        cerr << "FATAL (assert). Small immediate stated as destination register" << endl;
        return false;
    }

    // dest generally allowed to be either A regfile or a B regfile.
    if ((dest.file != QPUreg::A) && (dest.file != QPUreg::B)) {
        cerr << "FATAL (assert). Destination register not an A or B register file" << endl;
        return false;
    }


    tok = nextToken(stream, token_string, &stream);
    if (tok != COMMA)
        return false;

    tok = nextToken(stream, token_string, &stream);
    if (tok != WORD)
        return false;
    if (!parseRegister(token_string, r1)) {
        cerr << "FATAL (assert). Small immediate stated as first read register" << endl;
        return false;
    }

    // r1 generally allowed to be either A regfile or Accumulator
    if ((r1.file != QPUreg::A) && (r1.file != QPUreg::ACCUM)) {
        cerr << "FATAL (assert). First read argument not an A register file "
             << "or an Accumulator" << endl;
        return false;
    }


    tok = nextToken(stream, token_string, &stream);
    if (tok != COMMA)
        return false;

    tok = nextToken(stream, token_string, &stream);
    if (tok != WORD)
        return false;
    if (!parseRegister(token_string, r2)) {
        // As token_string does not represent a B or ACCUM register, it
        // needs to be a small immediate ('SMALL')
        r2.file = QPUreg::SMALL;

        uint32_t immediate = parseImmediate(token_string);

        // double check handle negative values
        if (immediate <= 63)
            r2.num = immediate;
        else {
            cerr << "ERROR. Stated small immediate larger than 63 (6 bits)" << endl;
            return false;
        }
    }

    // r2 generally allowed to be either B regfile, Accumulator or Small Immed
    if ((r2.file != QPUreg::B) && (r2.file != QPUreg::ACCUM)
        && (r2.file != QPUreg::SMALL)) {
        cerr << "FATAL (assert). Second read argument not a B register file "
             << "or an Accumulator or a Small Immediate" << endl;
        return false;
    }


    // advance stream pointer to end of whole pipeline instruction
    *stream_ptr = stream;

    return true;
}





/* Determines if the 'token_string' is an 'A' register, 'B' register or an
 * accumulator register and parses the number of the register. This information
 * is then saved in the QPUreg 'reg'
 */
bool parseRegister(const string& token_string, QPUreg& reg)
{
    if (token_string[0] != 'r') {
        return false;
    }

    int offset = 0;
    switch (token_string[1]) {
        case 'a':
            reg.file = QPUreg::A;
            offset = 2;
            break;
        case 'b':
            reg.file = QPUreg::B;
            offset = 2;
            break;

        default:
            reg.file = QPUreg::ACCUM;
            offset = 1;
    }

    // Parse token_string register number and save to reg.num. Then check for errors
    reg.num = strtol(token_string.c_str() + offset, NULL, 10);
    if (errno == EINVAL || reg.num > 63 || (reg.file == QPUreg::ACCUM && reg.num > 5)) {
        cerr << "ERROR. Register number not correct" << endl;
        exit(EXIT_FAILURE);
    }

    return true;
}





/* Determines if token_string value is hex, float or decimal. Then parses
 * it and returns the value
 */
uint32_t parseImmediate(const string& token_string)
{
    // if there is an 'x' we assume it's hex
    if (token_string.find_first_of("x") != string::npos) {
        uint32_t ret = strtoul(token_string.c_str(), NULL, 16);
        if (errno == EINVAL || errno == ERANGE) {
            cerr << "ERROR. Parsed Immediate invalid" << endl;
            exit(EXIT_FAILURE);
        }
        return ret;
    }

    // if there is an '.f' we assume it's float
    if (token_string.find_first_of(".f") != string::npos) {
        float f = strtof(token_string.c_str(), NULL);
        if (errno == ERANGE) {
            cerr << "ERROR. Parsed Immediate invalid" << endl;
            exit(EXIT_FAILURE);
        }
        return *(uint32_t*)&f;
    }

    // otherwise decimal
    uint32_t ret = strtoul(token_string.c_str(), NULL, 10);
    if (errno == EINVAL || errno == ERANGE) {
        cerr << "ERROR. Parsed Immediate invalid" << endl;
        exit(EXIT_FAILURE);
    }
    return ret;
}





/* Determines and returns if 'reg' is a regfile A, regfile B, a Small Immediate
 * or an Accum, in which case it returns the number of the Accum.
 */
uint8_t setALUMux(const QPUreg& reg)
{
    switch (reg.file) {
        case QPUreg::A:
            return 0x6;
        case QPUreg::B:
            return 0x7;
        case QPUreg::ACCUM:
            if (reg.num > 6 || reg.num < 0) {
                cerr << "Invalid accumulator register; out of range" << endl;
                exit(EXIT_FAILURE);
            }
            return reg.num;
        case QPUreg::SMALL:
            return 0x7;
    }
}






/*
/* Reads current line from 'ctx' and assembles the branch instruction into a
 * binary encoding of the instruction line which it then returns.
 * Examples for branch instruction:
 *      brr.zf ra39, rb39, loop;
 *      bra ra12, rb13, while;
 *      brr.ze ra39, rb39, for, ra1;
 */
uint64_t assembleBRANCHInstr(context& ctx, string token_string)
{
    uint64_t instruction;   // var for binary encoding of asm instruction line


    // Variables necessary to assemble instruction. For detailed description
    // see ARG, p.26
    uint8_t instruction_head;
    uint8_t cond_br;
    uint8_t rel;
    uint8_t reg;
    uint8_t raddr_a;
    uint8_t ws;
    uint8_t waddr_add;
    uint8_t waddr_mul;
    uint32_t immediate;


    // Variables necessary for this function
    uint32_t branch_target;     // ?
    token_t tok;                // var to save next token type
    QPUreg add_dest, mul_dest;  // ?




    // instruction_head, for QPU to recognize instruction as 'branch'
    instruction_head = 0xf;


    // rel, determines if branch target relative to PC+4
    rel = 1;
    if (token_string == "bra")
        rel = 0;


    // cond_br, determines the condition under which to branch
    cond_br = 15;               // default: always branch
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok == DOT) {
        // Branch is conditional
        nextToken(ctx.stream, token_string, &ctx.stream);
        cond_br = parseBranchCondition(token_string);
        tok = nextToken(ctx.stream, token_string, &ctx.stream);
    }


    // Parse add pipe write destination register
    if (tok != WORD) {
        cerr << "ERROR. Branch expecting destination register for add pipe."
             << endl;
        return (uint64_t) -1;
    }
    if (!parseRegister(token_string, add_dest)) {
        cerr << "FATAL (assert). Small immediate stated as destination register" << endl;
        return (uint64_t) -1;
    }


    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != COMMA)
        return (uint64_t) -1;


    // Parse mul pipe write destination register
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != WORD) {
        cerr << "ERROR. Branch expecting destination register for mul pipe."
             << endl;
        return (uint64_t) -1;
    }
    if (!parseRegister(token_string, mul_dest)) {
        cerr << "FATAL (assert). Small immediate stated as destination register" << endl;
        return (uint64_t) -1;
    }



    // Check if add/mul write destination register violate arch constraint
    if (((add_dest.file != QPUreg::A) && (add_dest.file != QPUreg::B)) ||
        ((mul_dest.file != QPUreg::A) && (mul_dest.file != QPUreg::B)) ||
        (add_dest.file == mul_dest.file)) {
        cerr << "ERROR. Add/Mul write destination register violate "
             << "architecture constraint." << endl;
        return (uint64_t) -1;
    }


    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != COMMA)
        return (uint64_t) -1;


    // Read Branch target label, then look it up in the 'labels' map. If the
    // label has not been encountered in the previous asm code, create a
    // reference for later relocation. If the label has already been
    // encountered, the corresponding pc is saved in labels. Make this the
    // branch target
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != WORD) {
        cerr << "ERROR. Expecting branch target label." << endl;
        return (uint64_t) -1;
    }
    if (ctx.labels.count(token_string) < 1) {
        relocation r;
        r.label = token_string;
        r.pc = ctx.pc;
        ctx.relocations.push_back(r);

        branch_target = 0xFFFFFFFF;             // No jump at all
    } else {
        branch_target = ctx.labels[token_string];
    }


    // If there's a fourth argument, it's the register determining the further
    // offset read into raddr_a. Read the register and set 'reg'. If no fourth
    // argument is present set reg to 0 so that raddr_a is ignored
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok == COMMA) {
        QPUreg offset_reg;

        tok = nextToken(ctx.stream, token_string, &ctx.stream);
        if (!parseRegister(token_string, offset_reg)) {
            cerr << "FATAL (assert). Small immediate stated as offset register" << endl;
            return (uint64_t) -1;
        }

        if (offset_reg.file != QPUreg::A) {
            cerr << "ERROR. Target offset reg must be regfile A" << endl;
            return (uint64_t) -1;
        }
        if (offset_reg.num > 31) {
            cerr << "ERROR. Target offset register must be < 32" << endl;
            return (uint64_t) -1;
        }

        reg = 1;
        raddr_a = offset_reg.num;
    } else {
        reg = 0;
        raddr_a = 0;
    }


    // ws, determine if 'write_swap' occurs (if add pipe writes to B reg file
    // and mul pipe writes to A reg file)
    if (add_dest.file == QPUreg::A)
        ws = 0;   // false
    else
        ws = 1;   // true


    // waddr_add, number of the register that is written to by add ALU.
    waddr_add = add_dest.num;


    // waddr_mul, number of the register that is written to by mul ALU.
    waddr_mul = mul_dest.num;


    // If branch is relative, the branch_target is relative to the link address
    // (current pc + 4*8 as branch happens 4 instructions after call and one
    // instructions equals 8 bytes)
    if (rel)
        immediate = branch_target - (ctx.pc + 4*8);
    else
        immediate = branch_target;



    cout << "Processing Branch at " << ctx.pc << " : " << branch_target
             << " : " << immediate << endl;



    // Encoding of all single instruction fields into one 64-bit instruction
    instruction = 0x0;
    instruction |= ((uint64_t)instruction_head << 60);
    instruction |= ((uint64_t)cond_br << 52);
    instruction |= ((uint64_t)rel << 51);
    instruction |= ((uint64_t)reg << 50);
    instruction |= ((uint64_t)raddr_a << 45);
    instruction |= ((uint64_t)ws << 44);
    instruction |= ((uint64_t)waddr_add << 38);
    instruction |= ((uint64_t)waddr_mul << 32);
    instruction |= ((uint64_t)immediate << 0);


    return instruction;
}





/* returns the representing cond_br number for a given branch condition
 * that is encoded in 'token_string'. For those branch conditions see ARG p.34
 */
uint8_t parseBranchCondition(const string& token_string)
{
    if (token_string == "zf")   // all Z flags set ("Z full")
        return 0;
    if (token_string == "ze")   // all Z flags clear ("Z empty")
        return 1;
    if (token_string == "zs")   // any Z flags set ("Z set")
        return 2;
    if (token_string == "zc")   // any Z flags clear ("Z clear")
        return 3;
    if (token_string == "nf")   // all N flags set ("N full")
        return 4;
    if (token_string == "ne")   // all N flags clear ("N empty")
        return 5;
    if (token_string == "ns")   // any N flags set ("N set")
        return 6;
    if (token_string == "nc")   // any N flags clear ("N clear")
        return 7;
    if (token_string == "cf")   // all C flags set ("C full")
        return 8;
    if (token_string == "ce")   // all C flags clear ("C empty")
        return 9;
    if (token_string == "cs")   // any C flags set ("C set")
        return 10;
    if (token_string == "cc")   // any C flags clear ("C clear")
        return 11;
    if (token_string == "*")    // always execute (unconditional)
        return 15;

    // throw some exceptions
    cerr << "FATAL (assert). Invalid branch condition: "
         << token_string << endl;
    exit(0);
}







/* Reads current line from 'ctx' and assembles the load immediate instruction
 * into a binary encoding of the instruction line which it then returns
 * Examples for load immediate instruction:
 *      ldi ra5, 0x12341234;
 *      ldi ra6, rb7, 0x56781234;
 *      ldi rb8, ra9, 0x0000f0f0;
 */
uint64_t assembleLDIInstr(context& ctx, string token_string)
{
    uint64_t instruction;   // var for binary encoding of asm instruction line


    // Variables necessary to assemble instruction. For detailed description
    // see ARG, p.26
    uint8_t instruction_head;
    uint8_t pm;
    uint8_t pack;
    uint8_t cond_add;
    uint8_t cond_mul;
    uint8_t sf;
    uint8_t ws;
    uint8_t waddr_add;
    uint8_t waddr_mul;
    uint32_t immediate;


    // Variables necessary for this function
    token_t tok;                // var to save next token type
    QPUreg add_dest, mul_dest;  // ?




    // Read next token and throw error if the 'ldi' instruction is not followed
    // by the first load target register (e.g. if condition present)
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != WORD) {
        cerr << "ERROR. Expecting the first target register after 'ldi'. Conditionals "
             << "for Immediate Loads are not supported in this assembler." << endl;
        return (uint64_t) -1;
    }
    if (!parseRegister(token_string, add_dest)) {
        cerr << "FATAL (assert). Small immediate stated as destination register" << endl;
        return (uint64_t) -1;
    }


    if (add_dest.file != QPUreg::A && add_dest.file != QPUreg::B) {
        cerr << "ERROR. Load target register has to be A/B regfile" << endl;
        return (uint64_t) -1;
    }


    // Expect Comma seperating first target register and second target
    // register or 32 bit immediate
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != COMMA)
        return (uint64_t) -1;


    // Next token string can be either the write register for the mul pipe or
    // the immediate
    tok = nextToken(ctx.stream, token_string, &ctx.stream);

    if (token_string[0] == 'r') {
        if (!parseRegister(token_string, mul_dest)) {
            cerr << "FATAL (assert). Small immediate stated as destination register" << endl;
            return (uint64_t) -1;
        }

        if (add_dest.file == mul_dest.file) {
            cerr << "ERROR. Add and Mul pipeline write to same registerfile "
                 << "in immediate load" << endl;
            return (uint64_t) -1;
        }
        if (mul_dest.file != QPUreg::A && mul_dest.file != QPUreg::B) {
            cerr << "ERROR. Load target reg has to be A/B regfile" << endl;
            return (uint64_t) -1;
        }

        // Expect Comma seperating second target register and 32 bit immediate
        tok = nextToken(ctx.stream, token_string, &ctx.stream);
        if (tok != COMMA)
            return (uint64_t) -1;

        tok = nextToken(ctx.stream, token_string, &ctx.stream);
    } else {
        // Automatically generate write for mul pipeline to a NOP register
        mul_dest.file = (add_dest.file == QPUreg::A) ? QPUreg::B : QPUreg::A;
        mul_dest.num = 39;
    }


    // Parse the immediate
    immediate = parseImmediate(token_string);


    // instruction_head, for QPU to recognize instruction as 'load imm 32'
    instruction_head = 0x70;


    // pm, functionality off
    pm = 0;         // special condition for pack/unpack


    // pack, functionality off
    pack = 0;       // no pack


    // cond_add, TODO: Description
    cond_add = 1;   // add condition set to 'always'


    // cond_mul, TODO: Description
    cond_mul = 1;   // mul condition set to 'always'


    // sf, Bit that determines if N, Z, C conditions will be updated
    sf = 1;


    // ws, determine if 'write_swap' occurs (if add pipe writes to B reg file
    // and mul pipe writes to A reg file)
    if (add_dest.file == QPUreg::A)
        ws = 0;   // false
    else
        ws = 1;   // true


    // waddr_add, number of the register that is written to by add ALU.
    waddr_add = add_dest.num;


    // waddr_mul, number of the register that is written to by mul ALU.
    waddr_mul = mul_dest.num;



    // Encoding of all single instruction fields into one 64-bit instruction
    instruction = 0x0;
    instruction |= ((uint64_t)instruction_head << 57);
    instruction |= ((uint64_t)pm << 56);
    instruction |= ((uint64_t)pack << 52);
    instruction |= ((uint64_t)cond_add << 49);
    instruction |= ((uint64_t)cond_mul << 46);
    instruction |= ((uint64_t)sf << 45);
    instruction |= ((uint64_t)ws << 44);
    instruction |= ((uint64_t)waddr_add << 38);
    instruction |= ((uint64_t)waddr_mul << 32);
    instruction |= ((uint64_t)immediate << 0);


    return instruction;
}





/* Reads current line from 'ctx' and assembles the sema instruction
 * into a binary encoding of the instruction line which it then returns.
 * The RPi GPU posseses 16 semaphores and each QPU will stall if it is
 * attempting to decrement a semaphore below 0 or increment it above 15
 * Examples for sema instruction:
 *      sema down, 0
 *      sema up, 7
 */
uint64_t assembleSEMAInstr(context& ctx, string token_string)
{
    uint64_t instruction;   // var for binary encoding of asm instruction line

    // Variables necessary to assemble instruction. For detailed description
    // see ARG, p.26
    uint8_t instruction_head;
    uint8_t pm;
    uint8_t pack;
    uint8_t cond_add;
    uint8_t cond_mul;
    uint8_t sf;
    uint8_t ws;
    uint8_t waddr_add;
    uint8_t waddr_mul;
    uint8_t sa;
    uint8_t semaphore;

    token_t tok;                // var to save next token type


    // instruction_head, for QPU to recognize instruction as 'Semaphore'
    instruction_head = 0x74;

    // pm, pack, cond_add, cond_mul, sf, ws, waddr_add, waddr_mul
    // all filled with standard values as ALU functionality useless with sema
    pm = 0;
    pack = 0;
    cond_add = 1;
    cond_mul = 1;
    sf = 0;
    ws = 0;
    waddr_add = 39;
    waddr_mul = 39;


    // sa, set to 0 if semaphore is incremented; set to 1 if semaphore is decremented
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (token_string == "up" || token_string == "release") {
        sa = 0;
    } else if (token_string == "down" || token_string == "acquire") {
        sa = 1;
    } else {
        cerr << "ERROR. Sema instruction is not followed by up/down or acquire/release" << endl;
        return (uint64_t) -1;
    }


    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    if (tok != COMMA) {
        return (uint64_t) -1;
    }


    // semaphore, stating the number of semaphore to be changed (from 0 to 15)
    tok = nextToken(ctx.stream, token_string, &ctx.stream);
    semaphore = (uint8_t) parseImmediate(token_string);
    if (semaphore < 0 || semaphore > 15) {
        cerr << "ERROR. Semaphore out of range" << endl;
        return (uint64_t) -1;
    }


    cout << "Processing semaphore at " << ctx.pc << " : " << semaphore
             << " : " << (int)sa << endl;


    // Encoding of all single instruction fields into one 64-bit instruction
    instruction = 0x0;
    instruction |= ((uint64_t)instruction_head << 57);
    instruction |= ((uint64_t)pm << 56);
    instruction |= ((uint64_t)pack << 52);
    instruction |= ((uint64_t)cond_add << 49);
    instruction |= ((uint64_t)cond_mul << 46);
    instruction |= ((uint64_t)sf << 45);
    instruction |= ((uint64_t)ws << 44);
    instruction |= ((uint64_t)waddr_add << 38);
    instruction |= ((uint64_t)waddr_mul << 32);
    instruction |= ((uint64_t)sa << 4);
    instruction |= ((uint64_t)semaphore << 0);


    return instruction;
}

