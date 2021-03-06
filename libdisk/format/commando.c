/*
 * disk/commando.c
 * 
 * Custom format as used on Commando by Elite/Capcom.
 * 
 * Written in 2012 by Keir Fraser
 * 
 * RAW TRACK LAYOUT:
 *  u16 0xa245,0x4489
 *  u16 trk_even,trk_odd
 *  u32 data_even[0x600]
 *  u32 csum_even
 *  u32 data_odd[0x600]
 *  u32 csum_odd
 *  Checksum is 1 - sum of all decoded longs.
 *  Track length is normal (not long)
 * 
 * TRKTYP_commando data layout:
 *  u8 sector_data[6*1024]
 */

#include <libdisk/util.h>
#include "../private.h"

static void *commando_write_raw(
    struct disk *d, unsigned int tracknr, struct stream *s)
{
    struct track_info *ti = &d->di->track[tracknr];

    while (stream_next_bit(s) != -1) {

        uint32_t csum, dat[0x601*2];
        uint16_t trk;
        unsigned int i;
        char *block;

        if (s->word != 0xa2454489)
            continue;

        ti->data_bitoff = s->index_offset - 31;

        if (stream_next_bytes(s, dat, 4) == -1)
            break;
        mfm_decode_bytes(bc_mfm_even_odd, 2, dat, &trk);
        trk = be16toh(trk);
        if (trk != tracknr)
            continue;

        if (stream_next_bytes(s, dat, sizeof(dat)) == -1)
            break;
        mfm_decode_bytes(bc_mfm_even_odd, sizeof(dat)/2, dat, dat);

        csum = ~0u;
        for (i = 0; i < 0x600; i++)
            csum -= be32toh(dat[i]);
        if (csum != be32toh(dat[0x600]))
            continue;

        block = memalloc(ti->len);
        memcpy(block, dat, ti->len);
        set_all_sectors_valid(ti);
        return block;
    }

    return NULL;
}

static void commando_read_raw(
    struct disk *d, unsigned int tracknr, struct tbuf *tbuf)
{
    struct track_info *ti = &d->di->track[tracknr];
    uint32_t csum, dat[0x601];
    unsigned int i;

    tbuf_bits(tbuf, SPEED_AVG, bc_raw, 32, 0xa2454489);
    tbuf_bits(tbuf, SPEED_AVG, bc_mfm_even_odd, 16, tracknr);

    memcpy(dat, ti->dat, ti->len);
    csum = ~0u;
    for (i = 0; i < 0x600; i++)
        csum -= be32toh(dat[i]);
    dat[0x600] = htobe32(csum);

    tbuf_bytes(tbuf, SPEED_AVG, bc_mfm_even_odd, 0x601*4, dat);
}

struct track_handler commando_handler = {
    .bytes_per_sector = 6*1024,
    .nr_sectors = 1,
    .write_raw = commando_write_raw,
    .read_raw = commando_read_raw
};

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
