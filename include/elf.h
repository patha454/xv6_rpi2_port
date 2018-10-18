// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  u_int32 magic;  // must equal ELF_MAGIC
  u_char8 elf[12];
  u_short16 type;
  u_short16 machine;
  u_int32 version;
  u_int32 entry;
  u_int32 phoff;
  u_int32 shoff;
  u_int32 flags;
  u_short16 ehsize;
  u_short16 phentsize;
  u_short16 phnum;
  u_short16 shentsize;
  u_short16 shnum;
  u_short16 shstrndx;
};

// Program section header
struct proghdr {
  u_int32 type;
  u_int32 off;
  u_int32 vaddr;
  u_int32 paddr;
  u_int32 filesz;
  u_int32 memsz;
  u_int32 flags;
  u_int32 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
