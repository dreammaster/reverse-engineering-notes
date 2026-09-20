/* Build/link smoke test for the ags/src reimplementation's third-party
 * library stack (Allegro 4.0.2, JGMOD, ALMP3, libcda). Exercises one real
 * entry point from each already-linked library and writes the result to
 * smoketest.log -- not a behavioral test, just proof the whole toolchain
 * (32-bit MinGW-w64) and library set actually builds and links together
 * before any engine reimplementation code is written against it.
 */

#include <stdio.h>
#include <allegro.h>
#include <jgmod.h>
#include <almp3.h>
#include <libcda.h>

int main(void)
{
   FILE *log = fopen("smoketest.log", "w");
   if (!log)
      return 1;

   fprintf(log, "allegro_init() -> %d (0=ok)\n", allegro_init());
   fprintf(log, "ALLEGRO_VERSION_STR: %s\n", ALLEGRO_VERSION_STR);

   fprintf(log, "JGMOD_VERSION_STR: %s\n", JGMOD_VERSION_STR);
   fprintf(log, "install_mod(0) -> %d\n", install_mod(0));
   remove_mod();

   /* almp3_create_mp3() has no documented NULL/zero-length contract --
    * unlike a real file-loading API, it doesn't validate its input before
    * reading from it, so passing NULL crashes rather than failing
    * gracefully. Pass a small non-NULL buffer of garbage bytes instead --
    * not a real MP3, so it should just be rejected as malformed. */
   {
      static const unsigned char fake_mp3_data[16] = { 0 };
      ALMP3_MP3 *fake_mp3;
      fprintf(log, "ALMP3_VERSION_STR: %s\n", ALMP3_VERSION_STR);
      fake_mp3 = almp3_create_mp3((void *)fake_mp3_data, sizeof(fake_mp3_data));
      fprintf(log, "almp3_create_mp3(<garbage>,16) -> %p (expect NULL, not a real MP3)\n",
              (void *)fake_mp3);
      if (fake_mp3)
         almp3_destroy_mp3(fake_mp3);
   }

   fprintf(log, "cd_init() -> %d\n", cd_init());
   fprintf(log, "cd_error: %s\n", cd_error);
   cd_exit();

   fprintf(log, "allegro_exit() -> done\n");
   allegro_exit();

   fprintf(log, "SMOKETEST OK\n");
   fclose(log);
   return 0;
}
END_OF_MAIN()
