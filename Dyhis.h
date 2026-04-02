/*
 * ============================================================================
 *  DYHIS - Decode Your Hex Instruction Stream
 *  Credits: phcla
 * ============================================================================
 *  Ultra-lightweight x86/x86-64 disassembler and instruction encoder.
 *  Zero dependencies. No libc required. Pure C99.
 *
 *  Usage:
 *    #include "dyhis.h"
 *
 *    // Disassemble
 *    dyhis_inst inst;
 *    int len = dyhis_decode(&inst, buf, buf_len, addr, DYHIS_MODE_64);
 *
 *    char text[128];
 *    dyhis_format(&inst, text, sizeof(text));
 *
 *    // Encode
 *    uint8_t out[15];
 *    int n = dyhis_encode(out, sizeof(out), "mov rax, rbx", DYHIS_MODE_64);
 *
 * ============================================================================
 */

#ifndef DYHIS_H
#define DYHIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Portable fixed-width types (no stdint.h needed) ────────────────────── */

#ifdef DYHIS_NO_STDINT
  typedef unsigned char       dyhis_u8;
  typedef unsigned short      dyhis_u16;
  typedef unsigned int        dyhis_u32;
  typedef unsigned long long  dyhis_u64;
  typedef signed char         dyhis_i8;
  typedef signed short        dyhis_i16;
  typedef signed int          dyhis_i32;
  typedef signed long long    dyhis_i64;
#else
  #include <stdint.h>
  typedef uint8_t   dyhis_u8;
  typedef uint16_t  dyhis_u16;
  typedef uint32_t  dyhis_u32;
  typedef uint64_t  dyhis_u64;
  typedef int8_t    dyhis_i8;
  typedef int16_t   dyhis_i16;
  typedef int32_t   dyhis_i32;
  typedef int64_t   dyhis_i64;
#endif

/* ── Constants ──────────────────────────────────────────────────────────── */

#define DYHIS_MAX_INST_LEN   15    /* Maximum x86 instruction length       */
#define DYHIS_MAX_OPERANDS    4    /* Maximum operands per instruction      */
#define DYHIS_MAX_PREFIXES    4    /* Maximum legacy prefixes               */
#define DYHIS_FMT_BUF_SIZE  256    /* Recommended format buffer size        */

/* ── Processor modes ────────────────────────────────────────────────────── */

typedef enum {
    DYHIS_MODE_16 = 16,
    DYHIS_MODE_32 = 32,
    DYHIS_MODE_64 = 64
} dyhis_mode;

/* ── Error codes ────────────────────────────────────────────────────────── */

typedef enum {
    DYHIS_OK            =  0,
    DYHIS_ERR_NULL      = -1,    /* NULL pointer argument                  */
    DYHIS_ERR_BUFSIZE   = -2,    /* Buffer too small                       */
    DYHIS_ERR_INVALID   = -3,    /* Invalid/unrecognized instruction       */
    DYHIS_ERR_TRUNCATED = -4,    /* Instruction truncated (need more data) */
    DYHIS_ERR_ENCODE    = -5,    /* Encoding failed                        */
    DYHIS_ERR_SYNTAX    = -6,    /* Assembly syntax error                  */
    DYHIS_ERR_OPERAND   = -7     /* Invalid operand                        */
} dyhis_error;

/* ── Register identifiers ───────────────────────────────────────────────── */

typedef enum {
    /* 8-bit low */
    DYHIS_REG_AL = 0, DYHIS_REG_CL, DYHIS_REG_DL, DYHIS_REG_BL,
    DYHIS_REG_AH, DYHIS_REG_CH, DYHIS_REG_DH, DYHIS_REG_BH,
    /* 8-bit (REX) */
    DYHIS_REG_SPL = 0x10, DYHIS_REG_BPL, DYHIS_REG_SIL, DYHIS_REG_DIL,
    DYHIS_REG_R8B, DYHIS_REG_R9B, DYHIS_REG_R10B, DYHIS_REG_R11B,
    DYHIS_REG_R12B, DYHIS_REG_R13B, DYHIS_REG_R14B, DYHIS_REG_R15B,
    /* 16-bit */
    DYHIS_REG_AX = 0x20, DYHIS_REG_CX, DYHIS_REG_DX, DYHIS_REG_BX,
    DYHIS_REG_SP, DYHIS_REG_BP, DYHIS_REG_SI, DYHIS_REG_DI,
    DYHIS_REG_R8W, DYHIS_REG_R9W, DYHIS_REG_R10W, DYHIS_REG_R11W,
    DYHIS_REG_R12W, DYHIS_REG_R13W, DYHIS_REG_R14W, DYHIS_REG_R15W,
    /* 32-bit */
    DYHIS_REG_EAX = 0x30, DYHIS_REG_ECX, DYHIS_REG_EDX, DYHIS_REG_EBX,
    DYHIS_REG_ESP, DYHIS_REG_EBP, DYHIS_REG_ESI, DYHIS_REG_EDI,
    DYHIS_REG_R8D, DYHIS_REG_R9D, DYHIS_REG_R10D, DYHIS_REG_R11D,
    DYHIS_REG_R12D, DYHIS_REG_R13D, DYHIS_REG_R14D, DYHIS_REG_R15D,
    /* 64-bit */
    DYHIS_REG_RAX = 0x40, DYHIS_REG_RCX, DYHIS_REG_RDX, DYHIS_REG_RBX,
    DYHIS_REG_RSP, DYHIS_REG_RBP, DYHIS_REG_RSI, DYHIS_REG_RDI,
    DYHIS_REG_R8, DYHIS_REG_R9, DYHIS_REG_R10, DYHIS_REG_R11,
    DYHIS_REG_R12, DYHIS_REG_R13, DYHIS_REG_R14, DYHIS_REG_R15,
    /* Segment */
    DYHIS_REG_ES = 0x50, DYHIS_REG_CS, DYHIS_REG_SS, DYHIS_REG_DS,
    DYHIS_REG_FS, DYHIS_REG_GS,
    /* Instruction pointer */
    DYHIS_REG_RIP = 0x60, DYHIS_REG_EIP,
    /* No register */
    DYHIS_REG_NONE = 0xFF
} dyhis_reg;

/* ── Operand types ──────────────────────────────────────────────────────── */

typedef enum {
    DYHIS_OP_NONE = 0,
    DYHIS_OP_REG,          /* Register                                     */
    DYHIS_OP_MEM,          /* Memory reference                             */
    DYHIS_OP_IMM,          /* Immediate value                              */
    DYHIS_OP_REL           /* Relative offset (branches)                   */
} dyhis_op_type;

/* ── Memory operand ─────────────────────────────────────────────────────── */

typedef struct {
    dyhis_u8  base;        /* Base register (DYHIS_REG_*)                  */
    dyhis_u8  index;       /* Index register (DYHIS_REG_NONE if unused)    */
    dyhis_u8  scale;       /* Scale: 1, 2, 4, or 8                        */
    dyhis_u8  seg;         /* Segment override (DYHIS_REG_NONE if default) */
    dyhis_i64 disp;        /* Displacement                                 */
    dyhis_u8  disp_size;   /* 0, 1, 2, or 4 bytes                         */
    dyhis_u8  size;        /* Memory access size in bytes                  */
} dyhis_mem;

/* ── Instruction operand ────────────────────────────────────────────────── */

typedef struct {
    dyhis_op_type type;
    dyhis_u8      size;    /* Operand size in bytes (1, 2, 4, 8, 16...)    */
    union {
        dyhis_u8  reg;     /* DYHIS_OP_REG:  register id                  */
        dyhis_mem mem;     /* DYHIS_OP_MEM:  memory operand               */
        dyhis_i64 imm;     /* DYHIS_OP_IMM:  immediate value              */
        dyhis_i64 rel;     /* DYHIS_OP_REL:  relative offset              */
    };
} dyhis_operand;

/* ── Prefix flags (bitfield) ────────────────────────────────────────────── */

#define DYHIS_PFX_LOCK      0x0001
#define DYHIS_PFX_REP       0x0002
#define DYHIS_PFX_REPNE     0x0004
#define DYHIS_PFX_SEG_CS    0x0008
#define DYHIS_PFX_SEG_SS    0x0010
#define DYHIS_PFX_SEG_DS    0x0020
#define DYHIS_PFX_SEG_ES    0x0040
#define DYHIS_PFX_SEG_FS    0x0080
#define DYHIS_PFX_SEG_GS    0x0100
#define DYHIS_PFX_OPSIZE    0x0200   /* 0x66 operand size override         */
#define DYHIS_PFX_ADDRSIZE  0x0400   /* 0x67 address size override         */
#define DYHIS_PFX_REX       0x0800   /* Has REX prefix                     */
#define DYHIS_PFX_REX_W     0x1000   /* REX.W bit                          */
#define DYHIS_PFX_REX_R     0x2000   /* REX.R bit                          */
#define DYHIS_PFX_REX_X     0x4000   /* REX.X bit                          */
#define DYHIS_PFX_REX_B     0x8000   /* REX.B bit                          */

/* ── Decoded instruction ────────────────────────────────────────────────── */

typedef struct {
    /* Address and raw bytes */
    dyhis_u64     address;                       /* Virtual address        */
    dyhis_u8      bytes[DYHIS_MAX_INST_LEN];     /* Raw instruction bytes  */
    dyhis_u8      length;                        /* Total byte length      */

    /* Prefixes */
    dyhis_u16     prefixes;                      /* DYHIS_PFX_* bitfield   */
    dyhis_u8      rex;                           /* Raw REX byte (0=none)  */

    /* Opcode */
    dyhis_u8      opcode[3];                     /* Opcode bytes           */
    dyhis_u8      opcode_len;                    /* 1, 2, or 3            */

    /* ModR/M and SIB */
    dyhis_u8      modrm;                         /* Raw ModR/M byte       */
    dyhis_u8      sib;                           /* Raw SIB byte          */
    dyhis_u8      has_modrm;                     /* 1 if present          */
    dyhis_u8      has_sib;                       /* 1 if present          */

    /* Mnemonic */
    const char   *mnemonic;                      /* e.g. "mov", "add"     */

    /* Operands */
    dyhis_operand operands[DYHIS_MAX_OPERANDS];
    dyhis_u8      num_operands;

    /* Processor mode used for decoding */
    dyhis_u8      mode;                          /* 16, 32, or 64         */
} dyhis_inst;

/* ── Version info ───────────────────────────────────────────────────────── */

#define DYHIS_VERSION_MAJOR  1
#define DYHIS_VERSION_MINOR  0
#define DYHIS_VERSION_PATCH  0
#define DYHIS_VERSION_STRING "1.0.0"

/* ══════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * dyhis_decode - Disassemble a single instruction from a byte buffer.
 *
 * @inst:     Output instruction structure (must not be NULL).
 * @buf:      Pointer to code bytes.
 * @buf_len:  Number of available bytes in buf.
 * @address:  Virtual address of the first byte (for display & RIP-relative).
 * @mode:     DYHIS_MODE_16, DYHIS_MODE_32, or DYHIS_MODE_64.
 *
 * Returns: instruction length in bytes (>0) on success, or negative
 *          dyhis_error on failure.
 */
int dyhis_decode(dyhis_inst *inst,
                 const dyhis_u8 *buf,
                 int buf_len,
                 dyhis_u64 address,
                 dyhis_mode mode);

/*
 * dyhis_format - Format a decoded instruction into human-readable text.
 *
 * @inst:      Decoded instruction (from dyhis_decode).
 * @buf:       Output character buffer.
 * @buf_size:  Size of output buffer.
 *
 * Returns: number of characters written (excluding NUL), or negative
 *          dyhis_error on failure.
 */
int dyhis_format(const dyhis_inst *inst, char *buf, int buf_size);

/*
 * dyhis_format_ex - Extended format with options.
 *
 * @inst:       Decoded instruction.
 * @buf:        Output character buffer.
 * @buf_size:   Size of output buffer.
 * @show_addr:  If non-zero, prepend "address: " to output.
 * @show_bytes: If non-zero, show raw hex bytes before mnemonic.
 * @uppercase:  If non-zero, use uppercase mnemonics.
 *
 * Returns: characters written or negative dyhis_error.
 */
int dyhis_format_ex(const dyhis_inst *inst,
                    char *buf, int buf_size,
                    int show_addr, int show_bytes, int uppercase);

/*
 * dyhis_encode - Assemble a single instruction from text.
 *
 * @out:       Output byte buffer.
 * @out_size:  Size of output buffer (at least 15).
 * @text:      Assembly text, e.g. "mov rax, rbx".
 * @mode:      Target processor mode.
 *
 * Returns: number of encoded bytes (>0) on success, or negative
 *          dyhis_error on failure.
 */
int dyhis_encode(dyhis_u8 *out, int out_size,
                 const char *text, dyhis_mode mode);

/*
 * dyhis_reg_name - Get the string name of a register.
 *
 * @reg:  Register identifier (DYHIS_REG_*).
 *
 * Returns: register name string, or "???" if unknown.
 */
const char *dyhis_reg_name(dyhis_u8 reg);

/*
 * dyhis_error_string - Get human-readable error description.
 *
 * @err:  Error code (negative value from API functions).
 *
 * Returns: static string describing the error.
 */
const char *dyhis_error_string(int err);

/*
 * dyhis_version - Get library version string.
 *
 * Returns: version string, e.g. "1.0.0".
 */
const char *dyhis_version(void);

#ifdef __cplusplus
}
#endif

#endif /* DYHIS_H */
