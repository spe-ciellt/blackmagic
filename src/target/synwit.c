/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2018, SMDprutser <smdprutser@smdprutser.nl>
 * Copyright (C) 2018, Stefan Petersen <spe@ciellt.se>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Description
   -----------

   This is an implementation of the target-specific functions for the
   Synwit SWM050.
*/

#include "general.h"
#include "target.h"
#include "target_internal.h"
#include "cortexm.h"


#define SYNWIT_CPUID     0xE000ED00
#define SYNWIT_FLASHREG1 0x1F000000
#define SYNWIT_FLASHREG2 0x1F000038
#define SYNWIT_FLASHKEY  0xAAAAAAAA
#define SYNWIT_SYS_CFG_0 0x400F0000
#define SYNWIT_SYS_DBLF  0x400F0008

/* Forward declarations */
static void synwit_add_flash(target *t,
                             uint32_t addr, size_t length, size_t erasesize);
static int synwit_flash_erase(struct target_flash *f,
                              target_addr addr, size_t len);
static int synwit_flash_write(struct target_flash *f,
                              target_addr dest, const void *src, size_t len);
bool synwit_probe(target *t);


static bool synwit_cmd_erase_mass(target *t);


const struct command_s synwit_cmd_list[] = {
        {"erase_mass", (cmd_handler)synwit_cmd_erase_mass,
         "Erase entire flash memory"},
        {NULL, NULL, NULL}
};


static void synwit_add_flash(target *t,
                             uint32_t addr, size_t length, size_t erasesize)
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

static int synwit_flash_erase(struct target_flash *f,
                              target_addr dest, size_t size)
{
        target *t = f->t;

        /* Set clock to 18 MHz */
        target_mem_write32(t, SYNWIT_SYS_CFG_0, 1);
        target_mem_write32(t, SYNWIT_SYS_DBLF, 0);

        /* We want to erase page */
        target_mem_write32(t, SYNWIT_FLASHREG1, 4);

        /* Select the right page */
        while(size) {
                target_mem_write32(t, dest, SYNWIT_FLASHKEY);
                platform_delay(1);
                size -= f->blocksize;
                dest += f->blocksize;
        }

        /* Close flash interface */
        target_mem_write32(t, SYNWIT_FLASHREG1, 0);

        return 0;
}

static int synwit_flash_write(struct target_flash *f,
                              target_addr dest, const void *src, size_t size)
{
        target *t = f->t;

        /* Set clock to 18 MHz */
        target_mem_write32(t, SYNWIT_SYS_CFG_0, 1);
        target_mem_write32(t, SYNWIT_SYS_DBLF, 0);

        /* We want to write bytes */
        target_mem_write32(t, SYNWIT_FLASHREG1, 1);

        /* Write data */
        target_mem_write(t, dest, src, size);

        /* Close flash interface */
        target_mem_write32(t, SYNWIT_FLASHREG1, 0);

        return 0;
}

bool synwit_probe(target *t)
{
        t->idcode = target_mem_read32(t, SYNWIT_CPUID);

        switch (t->idcode) {
        case 0x410CC200:
                t->driver = "Synwit SWM";
                target_add_ram(t, 0x20000000, 0x400);
                synwit_add_flash(t, 0x00000000, 0x2000, 0x200);
                target_add_commands(t, synwit_cmd_list, "synwit");
                return true;
        }

        return false;
}

static bool synwit_cmd_erase_mass(target *t)
{
        /* Set clock to 18 MHz */
        target_mem_write32(t, SYNWIT_SYS_CFG_0, 1);
        target_mem_write32(t, SYNWIT_SYS_DBLF, 0);

        /* Erase chip cmd */
        target_mem_write32(t, SYNWIT_FLASHREG1, 6);
        target_mem_write32(t, SYNWIT_FLASHREG2, 1);
        target_mem_write32(t, 0x0, SYNWIT_FLASHKEY);

        /* Delay 2170 on 18MHz, approx 1ms */
        platform_delay(1);

        /* Close flash interface */
        target_mem_write32(t, SYNWIT_FLASHREG1, 0);

        tc_printf(t, "Device is erased\n");

        return true;
}
