/*
 * Copyright (c) 2013 - 2016, Vladimir Vassilev, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */

#include <stdlib.h>
#include <string.h>
#include "yangcli_wordexp.h"

int yangcli_wordexp (const char* words, yangcli_wordexp_t* pwordexp, int flags)
{
    unsigned int i;
    unsigned int len;
    unsigned char quoted;
    pwordexp->we_wordv=(char**)malloc(1024*sizeof(char*));
    pwordexp->we_word_line_offset=(int*)malloc(1024*sizeof(int));

    pwordexp->we_wordc=0;
    
    quoted=0;
    for(i=0,len=0;i<strlen(words);i++){
        if(quoted==0 && (words[i]=='\'' || words[i]=='\"')) {
            quoted=words[i];
        } else if(quoted==words[i]) {
            quoted=0;
        }

        if(quoted==0 && words[i]==' ') {
            if(len>0) {
                pwordexp->we_word_line_offset[pwordexp->we_wordc]=i-len;
                pwordexp->we_wordv[pwordexp->we_wordc]=malloc(len+1);
                memcpy(pwordexp->we_wordv[pwordexp->we_wordc],&words[i-len],len);
                pwordexp->we_wordv[pwordexp->we_wordc][len]=0;
                pwordexp->we_wordc++;
            }
            len=0;
        } else {
            len++;
        }
    }
    /*last*/
    if(len>0) {
        pwordexp->we_word_line_offset[pwordexp->we_wordc]=i-len;
        pwordexp->we_wordv[pwordexp->we_wordc]=malloc(len+1);
        memcpy(pwordexp->we_wordv[pwordexp->we_wordc],&words[i-len],len);
        pwordexp->we_wordv[pwordexp->we_wordc][len]=0;
        pwordexp->we_wordc++;
    }
    return 0;
}
void yangcli_wordfree (yangcli_wordexp_t * pwordexp)
{
    free(pwordexp->we_word_line_offset);
    free(pwordexp->we_wordv);
}
