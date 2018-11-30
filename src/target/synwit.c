

#include "general.h"
#include "target.h"
#include "target_internal.h"
#include "cortexm.h"

//defines
#define SYNWIN_SWM050_CPUID	0xE000ED00
#define SYNWIN_SWM050_FLASHREG1	0x1F000000
#define SYNWIN_SWM050_FLASHREG2	0x1F000038
#define SYNWIN_SWM050_FLASHKEY	0xAAAAAAAA
#define SYNWIN_SWM050_SYS_CFG_0	0x400F0000
#define SYNWIN_SWM050_SYS_DBLF	0x400F0008

// declarations
static void synwit_add_flash(target *t, uint32_t addr, size_t length, size_t erasesize);
static int synwit_flash_erase(struct target_flash *f, target_addr addr, size_t len);
static int synwit_flash_write(struct target_flash *f, target_addr dest, const void *src, size_t len);
bool synwit_probe(target *t);


static bool synwit_cmd_erase_mass(target *t);

static bool synwit_cmd_write_test(target *t);
static bool synwit_cmd_erase_test(target *t);



const struct command_s synwit_cmd_list[] = {
	{"erase_mass", (cmd_handler)synwit_cmd_erase_mass, "Erase entire flash memory"},
	{"test1", (cmd_handler)synwit_cmd_write_test, "write test"},
	{"test2", (cmd_handler)synwit_cmd_erase_test, "erase test"},
	{NULL, NULL, NULL}
};


static void synwit_add_flash(target *t, uint32_t addr, size_t length, size_t erasesize)
{
	struct target_flash *f = calloc(1, sizeof(*f));

	f->start = addr;
	f->length = length;
	f->blocksize = erasesize;
	f->erase = synwit_flash_erase;
	f->write = synwit_flash_write;
	f->buf_size = erasesize;
	target_add_flash(t, f);
}

static int synwit_flash_erase(struct target_flash *f, target_addr addr, size_t len)
{
	target *t = f->t;

	// 18Mhz
	target_mem_write32(t, SYNWIN_SWM050_SYS_CFG_0, 1);
	target_mem_write32(t, SYNWIN_SWM050_SYS_DBLF, 0);

	// we want to erase page
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 4);

	// select right page
	while(len)
	{
		target_mem_write32(t, addr, SYNWIN_SWM050_FLASHKEY);
		platform_delay(1);

		len-=f->blocksize;
		addr+=f->blocksize;
	}

	// close flashinterface
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 0);

	return 0;
}

static int synwit_flash_write(struct target_flash *f,
                              target_addr dest,
                              const void *src,
                              size_t size)
{
	target *t = f->t;

	// 18MHz
	target_mem_write32(t, SYNWIN_SWM050_SYS_CFG_0, 1);
	target_mem_write32(t, SYNWIN_SWM050_SYS_DBLF, 0);

	// we want to write bytes
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 1);

	// Write data
	target_mem_write(t, dest, src, size);

	// close flashinterface
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 0);

	return 0;
}

bool synwit_probe(target *t)
{
	t->idcode = target_mem_read32(t, SYNWIN_SWM050_CPUID);

	switch(t->idcode)
	{
	case 0x410CC200:
		t->driver = "Synwit SWM050";
		target_add_ram(t, 0x20000000, 0x400);
		synwit_add_flash(t, 0x00000000, 0x2000, 0x200);
		target_add_commands(t, synwit_cmd_list, "synwit");
		return true;
	}

	return false;
}

static bool synwit_cmd_erase_mass(target *t)
{
	// 18Mhz
	target_mem_write32(t, SYNWIN_SWM050_SYS_CFG_0, 1);
	target_mem_write32(t, SYNWIN_SWM050_SYS_DBLF, 0);

	// erasechipcmd
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 6);
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG2, 1);
	target_mem_write32(t, 0x0, SYNWIN_SWM050_FLASHKEY);

	// delay 2170 on 18Mhz
	// appros 1ms
	platform_delay(1);

	// close flashinterface
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 0);

	// success?
	tc_printf(t, "Device is erased\n");

	return true;
}

static bool synwit_cmd_write_test(target *t)
{
	uint32_t i;

	// 18Mhz
	target_mem_write32(t, SYNWIN_SWM050_SYS_CFG_0, 1);
	target_mem_write32(t, SYNWIN_SWM050_SYS_DBLF, 0);

	// write bytes
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 1);

	for(i=0; i<2048; i++)
		target_mem_write32(t, 0x00000000+(4*i), i);

	// close flashinterface
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 0);

	return true;
}

static bool synwit_cmd_erase_test(target *t)
{
	// 18Mhz
	target_mem_write32(t, SYNWIN_SWM050_SYS_CFG_0, 1);
	target_mem_write32(t, SYNWIN_SWM050_SYS_DBLF, 0);

	// erase page
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 4);

	// select right page
	target_mem_write32(t, 0x00000000, SYNWIN_SWM050_FLASHKEY);
	platform_delay(1);

	// close flashinterface
	target_mem_write32(t, SYNWIN_SWM050_FLASHREG1, 0);

	return true;
}
