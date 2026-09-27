/* Test tool: a stereo Ogg Vorbis stream (440 Hz left, 660 Hz right, 22,050 Hz, 2 s) made
 * with libvorbis's reference encoder (ref/xiph), written to stdout. Used by
 * tests/halo_vorbis_test.c as its fixture. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vorbis/vorbisenc.h>

int main(void)
{
    vorbis_info vi;
    vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, 2, 22050, 0.4f)) return 1;
    vorbis_comment vc;
    vorbis_comment_init(&vc);
    vorbis_dsp_state vd;
    vorbis_block vb;
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);
    ogg_stream_state os;
    ogg_stream_init(&os, 1);
    ogg_packet h, hc, hk;
    vorbis_analysis_headerout(&vd, &vc, &h, &hc, &hk);
    ogg_stream_packetin(&os, &h); ogg_stream_packetin(&os, &hc); ogg_stream_packetin(&os, &hk);
    ogg_page og;
    while (ogg_stream_flush(&os, &og)) { fwrite(og.header, 1, (size_t)og.header_len, stdout); fwrite(og.body, 1, (size_t)og.body_len, stdout); }
    const int total = 44100;
    for (int done = 0; done <= total;) {
        int n = done < total ? (total - done < 1024 ? total - done : 1024) : 0;
        if (n) {
            float **buf = vorbis_analysis_buffer(&vd, n);
            for (int i = 0; i < n; i++) {
                double t = (double)(done + i) / 22050.0;
                buf[0][i] = (float)(0.125 * sin(2 * M_PI * 440 * t));
                buf[1][i] = (float)(0.125 * sin(2 * M_PI * 660 * t));
            }
        }
        vorbis_analysis_wrote(&vd, n);
        done += n ? n : 1;
        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);
            ogg_packet op;
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
                while (ogg_stream_pageout(&os, &og)) { fwrite(og.header, 1, (size_t)og.header_len, stdout); fwrite(og.body, 1, (size_t)og.body_len, stdout); }
            }
        }
    }
    while (ogg_stream_flush(&os, &og)) { fwrite(og.header, 1, (size_t)og.header_len, stdout); fwrite(og.body, 1, (size_t)og.body_len, stdout); }
    return 0;
}
