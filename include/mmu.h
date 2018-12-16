/**
 * @file mmu.h
 *
 * mmu.h provides constants for the ARM Memory Management unit, such
 * as page table attributes flags, and macros to manipulate page table
 * entries.
 *
 * The ARM page table is split into to layers: A 4096 entry page
 * directory and 256 entry page tables. Each page directory
 * contains attributes for 256 pages, and a pointer to the base
 * of the relevant page table. Each page table contains attributes
 * for a page, and a pointer to the base of the page. This allows
 * large blocks of memory to be controled in the page directory,
 * and a finer control to be done in the page table for each PDX
 * entry.
 *
 * ARMv7 supports a number of different page and section sizes
 * (Sections being the large chunks of memory managed at the
 * page directory level.) For simplicity, ARM xv6 uses only
 * 4096 byte pages and 1kB sections. This allows xv6 to manage,
 * in theory, (4096 page directory entries) * (256 page table entries)
 * (4096 bytes page) = 4GB of memory.
 *
 * However, ARM xv6 only uses the first 1024 page directory entries,
 * so that the page directory fits into a single page. This does mean
 * ARM xv6 can actually only address 1024 * 356 * 4096 = 1GB of memory.
 *
 * @note A virtual address 'va' has a three-part structure as follows:
 *
 *+--------12------+-------8--------+---------12----------+
 *| Page Directory |   Page Table   | Offset within Page  |
 *|      Index     |      Index     |                     |
 *+----------------+----------------+---------------------+
 * \--- PDX(va) --/ \--- PTX(va) --/
 *
 * @note The memory management system is poorly documented in XV6
 * documentation, and some of MIT's (x86 XV6) documentation is
 * irrelevant to the different ARM MMU architecture. Chapter 9 of
 * The ARMv7 Cortex-A Series Programmer's Guide is an abnormally
 * brief and clear guide to the ARM MMU, and highly recommended
 * reading you are unclear how the MMU and page tables in xv6 work.
 *
 * @see Chapter 9 of The Armv7 Cortex-A Series Programmer's Guide.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and refactoring.)
 *
 * @date 15/12/2018
 */


/** One megabyte (2^6 bytes), used as a convenient constant. */
#define MBYTE 0x100000


/** @def K_PDX_BASE - Physical address for the base of the
 * kernel's page directory.
 *
 * The kernel page table maps memory used for the kernel's
 * data and code, which is mapped into every process.
 *
 * @note K_PDX_BASE is listed in mmu.h as a comment for
 * reference. The symbol actually defined with a linker
 * symbol definition in the makefile.
 */
//Makefile #define K_PDX_BASE	0x4000


/**
 * @def K_PTX_BASE - Physical address of the base of the
 * kernel's page table.
 *
 * The xv6 kernel uses only a single page table, so the
 * kernel can only map 1MB for it's own use (PGSIZE * . This is fine:
 * The build size of the kernel is ~ 600kb (kernel7.img),
 * and the kernel should only allocate in the order of
 * tens of kilobytes for it's stack and heap.
 *
 * @note K_PTX_BASE is listed in mmu.h as a comment for
 * reference. The symbol is actually defined with a linker
 * symbol definition in the makefile.
 */
//Makefile #define K_PTX_BASE 0x3000


/**
 * @def CACHE_LINE_SIZE - The unit size for cache entries,
 * in bytes.
 *
 * The CPU memory cache operates in units of lines. Memory
 * must be moved between the RAM and cache in units of a
 * fixed size, called 'lines.'
 *
 * In ARM, the line size is 32 bytes.
 *
 * @note The term "lines" here means as in "lines of text",
 * like memory is divided into "pages." The term has
 * nothing to do with bus line widths...
 */
#define CACHE_LINE_SIZE 32


/**
 * @def UNMAPPED - Indicates a page table or page directory
 * entry is unmapped.
 *
 * If bits [1:0] in a page table or directory entry are
 * both zero, the MMU considers the entry to to refer to
 * any memory and will generate a fault if any process
 * attempts to access the memory.
 */
#define UNMAPPED 0x0


/**
 * @def PDX_ATRB_PTX_ENTRY - Indicates a page directory
 * entry refers to a (secondary) page table.
 *
 * PDX_ATRB_PTX_ENTRY ("Page Directory Attribute: PTX
 * Entry") is a PDX flag which indicates that an entry in
 * the page directory refers to a secondary page table,
 * where the page table's base is given in the page
 * directory entry along with other attributes. This is
 * opposed to a directly mapped 1MB section or 16MB
 * supersection.
 *
 * This flag can be OR'ed onto a a page directory entry
 * to set the flag.
 */
#define PDX_ATRB_PTX_ENTRY (1)


/**
 * @def PDX_ATRB_SECTION_ENTRY - Indicates a page directory
 * entry refers directly to a 1MB or 16MB section.
 *
 * PDX_ATRB_SECTION_ENTRY is a PDX flag which indicates the
 * entry directly maps a 1MB or 16MB section of memory, where
 * the section's base is given in the page directory entry.
 *
 * This is opposed to a directory entry referring to a
 * (secondary) page table which would allow more fine grained
 * memory control at a memory and performance cost.
 */
#define PDX_ATRB_SECTION_ENTRY (2)


/**
 * @def PTX_ATRB_LARGE - Refers to a large 64KB page.
 *
 * PTX_ATRB_LARGE (PTX Attribute: Large) is the bit flag
 * used to denote a page entry refers to a 64KB page.
 *
 * PTX_ATRB_LARGE can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the
 * MMU.
 *
 * @note With large pages, most significant 16 bits
 * indicate the physical address of the page - The other
 * 16 bits of the page's base address are zero, as a 64KB
 * page must be 64KB aligned.
 */
#define PTX_ATRB_LARGE 0x1


/**
 * @def PTX_ATRB_SMALL - Refers to a small 4KB page.
 *
 * PTX_ATRB_SMALL (PTX Attribute: Small) is the bit flag
 * used to denote a 4KB page in a page table entry.
 *
 * PTX_ATRB_SMALL can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @note With small pages, the most significant 20 bits of
 * the page table entry indicate the physical address of
 * the page. The the 12 less significant bits of the page's
 * physical address are zero, as a 4KB page must be 4KB
 * aligned.
 */
#define PTX_ATRB_SMALL 0x2


/**
 * @def PTX_ATRB_BUFFERED - Indicates memory can be
 * buffered.
 *
 * PTX_ATRB_BUFFERED (PTX Attribute: Buffered) indicates
 * memory in the page mapped by the page table entry can
 * be buffered before writing.
 *
 * PTX_ATRB_BUFFERED can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @note Situations where buffering is undesirable include
 * memory-mapped I/O devices, where writing to the address
 * causes side effects on the device that are assumed to
 * happen immediately.
 */
#define PTX_ATRB_BUFFERED 0x4


/**
 * @def PTX_ATRB_CACHED - Indicates memory can be cached.
 *
 * PTX_ATRB_CACHED (PTX Attribute: Cached) indicates
 * memory in the page mapped by the page table entry
 * can be cached for reading.
 *
 * PTX_ATRB_CACHED can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @note Situations where caching is undesirable include
 * memory-mapped I/O devices, where the value associated
 * with a memory address could change without a CPU write
 * (and so unseen/transparent to the CPU) creating a
 * dirty read error if cached.
 */
#define PTX_ATRB_CACHED 0x8


/**
 * @def PTX_ATRB_APX - Indicates the memory is read only.
 *
 * PTX_ATRB_APX (PTX Attribute: Access Permission Extended)
 * marks a memory page as highly restricted. The exact
 * restrictions also depend on the the AP bits are set,
 * but in no case can any CPU mode write to pages where
 * the APX bit is set.
 *
 * PTX_ATRB_APX can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @see Section 9.6.1 (Memory Access Permissions) in the
 * Cortex-A series Programmer's Guide, for the full
 * restrictions of different AP and APX bit settings.
 */
#define PTX_ATRB_APX (1 << 9)


/**
 * @def PTX_ATRB_SHAREABLE - Indicates if the page is
 * shared between CPU cores.
 *
 * PTX_ATRB_SHAREABLE (PTX Attribute: Shareable) marks a
 * page as shareable between CPUs/cores. Shareable memory
 * is guaranteed to be maintained as coherent across
 * all cores by the hardware.
 *
 * PTX_ATRB_SHAREABLE can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @note If PTX_ATRB_SHARABLE is not set, but the page is
 * used by multiple cores, individual cores must perform
 * appropriate cache maintenance and barrier operations
 * at the software level.
 */
#define PTX_ATRB_SHAREABLE (1 << 10)


/**
 * @def PTX_ATRB_nG - Indicates if a page is owned by a
 * particular application.
 *
 * PTX_ATRB_nG (PTX Attribute: Non-Global) allows us to
 * mark a page as owned by a particular application
 * specific ID (ASID.) In this case, for a page table
 * entry to match, the virtual address must match the
 * page table entry, and the ASID in the PTX entry must
 * match the current ASID register (CP15 register c13).
 * ASIDs can be assigned freely by the operating system.
 *
 * PTX_ATRB_nG can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @warning ARM xv6 does not use ARM's ASID features.
 */
#define PTX_ATRB_nG (1 << 11)


/**
 * @def PTX_ATRB_XN - Marks a page as never to be executed.
 *
 * PTX_ATRB_XN (PTX Attribute: Never Execute) prevents
 * memory stored in the associated page from being
 * executed - either explicitly or by speculative
 * execution.
 *
 * The MMU will generate a prefetch abort if an instruction
 * in a page with Never Execute set is pipelined for
 * execution.
 *
 * PTX_ATRB_XN can be bitwise OR'ed with other PTX
 * attributes to configure a page's attributes in the MMU.
 *
 * @warning The XN bit is in a different position if large
 * pages are being used. xv6 only uses small pages.
 */
#define PTX_ATRB_XN (0x1)


/**
 * @def PDX_ATRB_DOMAIN0 - Marks a page directory as
 * belonging to domain zero.
 *
 * AMRv7 has deprecated the domain system and it will
 * eventually be removed. However, for memory permissions
 * to be enforced, each PDX must have a domain set, and
 * the Domain Access Control Register (DACR - CP15 c3)
 * must be set.
 *
 * ARM recommends using domain zero for all page dirs, and
 * setting all DACR fields to client.
 *
 * @note Apparently ARM aims to replace the page
 * directory level domain system with the page table level
 * ASID system (see the discussion in the PTX_ATRB_nG
 * documentation.)
 */
#define PDX_ATRB_DOMAIN0 0


/**
 *  @def PTX_ATRB_NOACCESS - AP (Access Permission) bits
 *  which mark a page as inaccessible to both the kernel
 *  and user.
 *
 *  @note The AP bits are not the least significant bits in
 *  a page table entry. Use the PDX_AP and PTX_AP macro
 *  functions to get a bitfield which can be XOR'ed onto
 *  a page table entry.
 */
#define PTX_ATRB_NOACCESS 0


/**
 *  @def PTX_ATRB_KRW - AP (Access Permission) bits
 *  which mark a page as accessible to the privileged CPU
 *  modes only.
 *
 *  PTX_ATRB_KRW (Kenrel Read/Write) is an AP bit field
 *  value which makes a page table entry usable from the
 *  privileged CPU modes only. On xv6, only the kernel uses
 *
 *  @note The AP bits are not the least significant bits in
 *  a page table entry. Use the PDX_AP and PTX_AP macro
 *  functions to get a bitfield which can be XOR'ed onto
 *  a page table entry.
 */
#define PTX_ATRB_KRW 1


/**
 *  @def PTX_ATRB_UAP - AP (Access Permission) bits
 *  which mark a page as accessible to the privileged CPU
 *  modes only.
 *
 *  PTX_ATRB_UAP (User access permission) is an AP bit field
 *  value which makes a page table entry readable from
 *  unprivileged CPU modes, and readable & writable from
 *  privileged modes.
 *
 *  @note The AP bits are not the least significant bits in
 *  a page table entry. Use the PDX_AP and PTX_AP macro
 *  functions to get a bitfield which can be XOR'ed onto
 *  a page table entry.
 */
#define PTX_ATRB_UAP 2


/**
 *  @def PTX_ATRB_UARW - AP (Access Permission) bits
 *  which mark a page as accessible to the privileged CPU
 *  modes only.
 *
 *  PTX_ATRB_URW (User read/write) is an AP bit field
 *  value which makes a page table entry both readable and
 *  writeable from all CPU modes.
 *
 *  @note The AP bits are not the least significant bits in
 *  a page table entry. Use the PDX_AP and PTX_AP macro
 *  functions to get a bitfield which can be XOR'ed onto
 *  a page table entry.
 */
#define PTX_ATRB_URW 3


/**
 * @def PTX_ATRB_ACCESS_PERM - Generates the AP bits
 * (access permission) for PDX or PTX entries.
 *
 * @param n - Related to the offset of the AP bits in the
 * entry.
 * @param ap - The access permission attribute.
 * @return The set AP bits for a PTX or PDX entry.
 */
#define PTX_ATRB_ACCESS_PERM(n, ap) (((ap) & 3) << (((n) * 2) + 4))


/**
 * @def PXT_ATRB_AP - Generates access permission bits for
 * a page directory entry.
 *
 * PDX_ATRB_AP (Page Directory Attribute: Access Permission)
 * generates access permission attribute bitfields which
 * define how privileged and unprivileged CPU modes can
 * access (or not) sections of memory defined by a page
 * directory entry. In xv6, only the kernel uses privileged
 * CPU modes, so this can be used for kernel and userspace
 * access control.
 *
 * PDX_ATRB_AP can be XOR'ed onto a page directory entry to
 * set the access permissions for that page directory
 * entry.
 *
 * @note The access permission bits alone do not define
 * read/write permissions completely. The APX bit can
 * modify the meaning of the AP bits to make access more
 * restrictive.
 *
 * @see Table 9-1 of the ARMv7 Programmer's Guide for the
 * meaning of each AP and APX bit setting.
 *
 * @param ap The access permission to generate a bitfield
 * for. This should be one of PTX_ATRB_URW, PTX_ATRB_UAP,
 * PTX_ATRB_KRW, and PTX_ATRB_NOACCESS.
 * @return The PDX AP bit field for the given access
 * permission.
 */
#define PDX_ATRB_AP(ap) (PTX_ATRB_ACCESS_PERM(3, (ap)))


/**
 * @def PTX_ATRB_AP - Generates access permission bits for
 * a page table entry.
 *
 * PTX_ATRB_AP (Page Table Attribute: Access Permission)
 * generates access permission attribute bitfields which
 * define how privileged and unprivileged CPU modes can
 * access (or not) sections of memory defined by a page
 * table entry. In xv6, only the kernel uses privileged
 * CPU modes, so this can be used for kernel and userspace
 * access control.
 *
 * PTX_ATRB_AP can be XOR'ed onto a page directory entry to
 * set the access permissions for that page table
 * entry.
 *
 * @note The access permission bits alone do not define
 * read/write permissions completely. The APX bit can
 * modify the meaning of the AP bits to make access more
 * restrictive.
 *
 * @see Table 9-1 of the ARMv7 Programmer's Guide for the
 * meaning of each AP and APX bit setting.
 *
 * @param ap The access permission to generate a bitfield
 * for. This should be one of PTX_ATRB_URW, PTX_ATRB_UAP,
 * PTX_ATRB_KRW, and PTX_ATRB_NOACCESS.
 * @return The PTX AP bit field for the given access
 * permission.
 */
#define PTX_ATRB_AP(ap) (PTX_ATRB_ACCESS_PERM(3, (ap)) \
						| PTX_ATRB_ACCESS_PERM(2, (ap)) \
						| PTX_ATRB_ACCESS_PERM(1, (ap)) \
						| PTX_ATRB_ACCESS_PERM(0, (ap)))


/**
 * @def HVECTORS - High memory address to map to trap
 * vectors to.
 *
 * HVECTORS (High trap Vectors) is a mapping used to store
 * the trap vectors it the high memory range (> 0x80000000)
 * used for the kernel in xv6. The trap vectors are mapped
 * twice (A high kernel address, and a low address in
 * user space.)
 *
 */
#define HVECTORS 0xffff0000


/**
 * @def - The PDX macro generates the page directory index
 * assosiated with a virtual address.
 *
 * PDX ("Page Directory Index") fetches the bits from a
 * virtual address which select which page directory
 * index
 *
 * The page directory index is a 12 bit index that
 * describes which entry in the page directory is
 * associated with virtual address 'va'
 *
 * @param va - The virtual address to compute the page
 * directory index for.
 */
#define PDX(va) (((u_int32) (va) >> PDXSHIFT) & 0xFFF)


/**
 * @def PTX - The PTX macro generates the page table index
 * from a virtual address.
 *
 * The page table index is an 8 bit index that describes
 * which entry in the corresponding page table is
 * associated with virtual address 'va'.
 *
 * @param va - The virtual address to compute the page
 * table index for.
 */
#define PTX(va) (((u_int32) (va) >> PTXSHIFT) & 0xFF)


/**
 * @def VIRTUAL_ADDR - Constructs a virtual address from
 * page table indexes and the offset.
 *
 * VIRTUAL_ADDR computes the virtual address for a given
 * page directory entry, page table entry, and offset
 * inside a page.
 *
 * @param pdx - The page directory index of the memory
 * @param ptx - The page table index of the memory.
 * @param offset - The memory's inside its page.
 *
 * @return - The virtual address for the specified memory.
 */
#define VIRTUAL_ADDR(pdx, ptx, offset) ((uint) ((pdx) << PDXSHIFT \
                                        | (ptx) << PTXSHIFT \
                                        | (offset)))


/**
 * @def PTE_ADDR - Returns the physical address
 * associated with a page table entry.
 *
 * @note This function only works for second level page
 * table entries. The page directory (first level page
 * table) has 10 bits of flags, while the page table
 * has 12 bits of flags, hence the non-compatibility.
 * Also note these bit numbers are only apply to
 * a page directory pointing to 2nd level page tables,
 * and page tables pointing to small 4KB pages. ARM has
 * several other MMU options with different PDX/PTX layout
 * formats.
 *
 * @param pte - The page table entry index.
 * @return - The physical address mapped at index 'pte.'
 */
#define PTE_ADDR(pte) ((u_int32)(pte) & ~0xFFF)


/**
 * @def - Return the page table entry flags associated
 * with a page table entry.
 *
 *  @note This function only works for second level page
 * table entries. The page directory (first level page
 * table) has 10 bits of flags, while the page table
 * has 12 bits of flags, hence the non-compatibility.
 * Also note these bit numbers are only apply to
 * a page directory pointing to 2nd level page tables,
 * and page tables pointing to small 4KB pages. ARM has
 * several other MMU options with different PDX/PTX layout
 * formats.
 *
 * @param pte - The page table entry index.
 * @return - The page table flags for the DTX entry at
 * index 'pte.'
 */
#define PTE_FLAGS(pte) ((u_int32)(pte) &  0xFFF)


/**
 * @def N_PD_ENTRIES - The number of page directory entries
 * in a page directory.
 *
 * @warning - This value may be incorrect. The ARMv7
 * Programmer's Guide asserts there are 4096 4-byte entries
 * in an ARM page directory. Perhaps this value not ported
 * from x86 properly. This argument is reinforced by the value of
 * PDXSHIFT, which implies a 12 bit PDX index ( = 2 ^ 12
 * entries = 4096 entries.)
 */
#define N_PD_ENTRIES 1024


/**
 * @def N_PT_ENTRIES - The number of page table entries in
 * a page table.
 *
 * @warning - This value may be incorrect. The ARMv7
 * Programmer's Guide asserts there are 256 4-byte entries
 * in an ARM page table. Perhaps this value was not ported
 * from x86 properly. This argument is reinforced by the
 * value of PTXSHIFT and PDXSHIFT, which imply an 8 bit
 * page table index ( = 2 ^ 8 entries = 256 entries.)
 */
#define N_PT_ENTRIES 1024


/**
 * @def PGSIZE - The size of a memory page, in bytes.
 *
 * PGSIZE assumes that only ARM's small pages are being
 * used. ARMv7 give us several options, including 4Kb and
 * 64Kb pages (referenced from the page table) and 16Mb or
 * 1Mb sections (referenced directly from the page
 * directory.)
 */
#define PGSIZE 4096


/**
 * @def PTXSHIFT - Offset of the page table index in a
 * virtual address.
 */
#define PTXSHIFT 12


/**
 * @def PDXSHIFT - Offset of the page directory index in a
 * virtual address.
 */
#define PDXSHIFT 20


/**
 * @def PG_ROUND_UP - Rounds up a number of bytes to a size
 * which is aligned to a page boundary.
 */
#define PG_ROUND_UP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))


/**
 * @def PG_ROUND_DOWN - Rounds down a number of bytes to a
 * size which is aligned to a page boundary.
 */
#define PG_ROUND_DOWN(sz) (((sz)) & ~(PGSIZE - 1))


/**
 * @def PDGIR_BASE - Returns the virtual address which the
 * base of the page directory is located at.
 */
#define PGDIR_BASE P2V(K_PDX_BASE)


/**
 * @def KVM_PDX_ATRB - Page directory attributes for virtual
 * memory mapped to the kernel.
 *
 * KVM_PDX_ATRB ("Kernel Virtual Memory Page Directory
 * entry Attributes") contains page directory flags used
 * for kernel virtual memory.
 *
 * @note KVM_PDX_ATRB is unused in ARM xv6.
 */
#define KVM_PDX_ATRB (PDX_ATRB_DOMAIN0 \
                    | PDX_ATRB_AP(PTX_ATRB_URW) \
                    | PDX_ATRB_SECTION_ENTRY \
                    | PTX_ATRB_CACHED \
                    | PTX_ATRB_BUFFERED)


/**
 * @def UVM_PDX_ATRB - Page directory attributes for virtual
 * memory mapped to user programs.
 *
 * UVM_PDX_ATRB ("User Virtual Memory Page Directory entry
 * Attributes") contains page directory bit flags used for
 * memory mapped to user programs in xv6.
 *
 * @note The attributes are very permissive - Most
 * properties are defined for finer-grained blocks of
 * memory in the page table entries.
 *
 * @see UVM_PTX_ATRB, where more restrictive attributes are
 * defined at the page table level.
 *
 */
#define UVM_PDX_ATRB (PDX_ATRB_DOMAIN0 | PDX_ATRB_PTX_ENTRY)


/**
 * @def UVM_PTX_ATRB - Page directory attributes for user
 * programs' virtual memory.
 */
#define UVM_PTX_ATRB ((PTX_ATRB_AP(PTX_ATRB_URW) ^ PTX_ATRB_APX) \
                    | PTX_ATRB_CACHED \
                    | PTX_ATRB_BUFFERED  \
                    | PTX_ATRB_SMALL)

