#include <stdlib.h>
#include <assert.h>
#include "yangcli_wordexp.h"

int main(char** argv, unsigned int argc)
{
    int ret;
    yangcli_wordexp_t my_wordexp;
    /* #1 - basic */
    ret = yangcli_wordexp ("connect user=root server=192.168.209.154", &my_wordexp, 0);
    assert(ret==0);
    assert(my_wordexp.we_wordc==3);
    assert(0==strcmp(my_wordexp.we_wordv[0],"connect"));
    assert(my_wordexp.we_word_line_offset[0]==0);
    assert(0==strcmp(my_wordexp.we_wordv[1],"user=root"));
    assert(my_wordexp.we_word_line_offset[1]==8);
    assert(0==strcmp(my_wordexp.we_wordv[2],"server=192.168.209.154"));
    assert(my_wordexp.we_word_line_offset[2]==18);
    yangcli_wordfree(&my_wordexp);

    /* #2 - striping spaces between tokens, ignoring spaces in quotes */
    ret = yangcli_wordexp ("  xget  /interfaces/interface[name='blah blah'] ", &my_wordexp, 0);
    assert(ret==0);
    assert(my_wordexp.we_wordc==2);
    assert(0==strcmp(my_wordexp.we_wordv[0],"xget"));
    assert(my_wordexp.we_word_line_offset[0]==2);
    assert(0==strcmp(my_wordexp.we_wordv[1],"/interfaces/interface[name='blah blah']"));
    assert(my_wordexp.we_word_line_offset[1]==8);
    yangcli_wordfree(&my_wordexp);

    return 0;
}
