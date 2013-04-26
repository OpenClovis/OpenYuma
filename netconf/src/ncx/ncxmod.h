/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
#ifndef _H_ncxmod
#define _H_ncxmod

/*  FILE: ncxmod.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Module Load Manager
  
     - manages NCX module search path
     - loads NCX module files by module name

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
10-nov-05    abb      Begun
22-jan-08    abb      Add support for YANG import and include
                      Unlike NCX, forward references are allowed
                      so import/include loops have to be tracked
                      and prevented
16-feb-08   abb       Changed environment variables from NCX to YANG
                      Added YANG_INSTALL envvar as well.
22-jul-08   abb       Remove NCX support -- YANG only from now on
06-oct-09   abb       Change YANG_ env vars to YUMA_ 
*/

#include <xmlstring.h>

#ifndef _H_cap
#include "cap.h"
#endif

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* max user-configurable directories for NCX and YANG modules */
#define NCXMOD_MAX_SEARCHPATH   64

/* maximum abolute filespec */
#define NCXMOD_MAX_FSPEC_LEN 2047

/* path, file separator char */
#define NCXMOD_PSCHAR   '/'

#define NCXMOD_HMCHAR   '~'

#define NCXMOD_ENVCHAR '$'

#define NCXMOD_DOTCHAR '.'

/* name of the NCX module containing agent boot parameters
 * loaded during startup 
 */
#define NCXMOD_NETCONFD   (const xmlChar *)"netconfd"

#define NCXMOD_NCX        (const xmlChar *)"yuma-ncx"

#define NCXMOD_YUMA_NACM  (const xmlChar *)"yuma-nacm"

#define NCXMOD_WITH_DEFAULTS (const xmlChar *)"ietf-netconf-with-defaults"

#define NCXMOD_YANGCLI (const xmlChar *)"yangcli"

/* name of the NETCONF module containing NETCONF protocol definitions,
 * that is loaded by default during startup 
 */
#define NCXMOD_NETCONF        (const xmlChar *)"yuma-netconf"

#define NCXMOD_YUMA_NETCONF   (const xmlChar *)"yuma-netconf"

#define NCXMOD_IETF_NETCONF   (const xmlChar *)"ietf-netconf"

#define NCXMOD_IETF_YANG_TYPES   (const xmlChar *)"ietf-yang-types"


#define NCXMOD_IETF_NETCONF_STATE (const xmlChar *)"ietf-netconf-monitoring"

/* name of the NCX modules directory appended when YUMA_HOME or HOME
 * ENV vars used to construct NCX module filespec
 */
#define NCXMOD_DIR            (const xmlChar *)"modules"

/* name of the data direectory when YUMA_HOME or HOME
 * ENV vars used to construct an NCX filespec
 */
#define NCXMOD_DATA_DIR        (const xmlChar *)"data"

/* name of the scripts direectory when YUMA_HOME or HOME
 * ENV vars used to construct a NCX filespec
 */
#define NCXMOD_SCRIPT_DIR       (const xmlChar *)"scripts"

/* STD Environment Variable for user home directory */
#define NCXMOD_PWD          "PWD"

/* STD Environment Variable for user home directory */
#define USER_HOME           "HOME"

/* NCX Environment Variable for YANG/NCX user work home directory */
#define NCXMOD_HOME         "YUMA_HOME"

/* NCX Environment Variable for tools install directory
 * The default is /usr/share/yuma
 */
#define NCXMOD_INSTALL   "YUMA_INSTALL"

/* !! should import this from make !! */
#ifdef FREEBSD
#define NCXMOD_DEFAULT_INSTALL (const xmlChar *)"/usr/local/share/yuma"
#else
#define NCXMOD_DEFAULT_INSTALL (const xmlChar *)"/usr/share/yuma"
#endif

/* !! should import this from make !! */
#define NCXMOD_DEFAULT_YUMALIB64 (const xmlChar *)"/usr/lib64/yuma"

#define NCXMOD_DEFAULT_YUMALIB (const xmlChar *)"/usr/lib/yuma"


/* !! should import this from make !! */
#define NCXMOD_ETC_DATA (const xmlChar *)"/etc/yuma"


/* NCX Environment Variable for MODULE search path */
#define NCXMOD_MODPATH      "YUMA_MODPATH"

/* NCX Environment Variable for DATA search path */
#define NCXMOD_DATAPATH      "YUMA_DATAPATH"

/* NCX Environment Variable for SCRIPTS search path */
#define NCXMOD_RUNPATH      "YUMA_RUNPATH"

/* per user yangcli internal data home when $HOME defined */
#define NCXMOD_YUMA_DIR (const xmlChar *)"~/.yuma"

/* per user yangcli internal data home when $HOME not defined */
#define NCXMOD_TEMP_YUMA_DIR (const xmlChar *)"/tmp/yuma"

/* Yuma work directory name */
#define NCXMOD_YUMA_DIRNAME    (const xmlChar *)".yuma"

/* sub-directory name yangcli uses to store local per-session workdirs
 * appended to ncxmod_yumadir_path
 */
#define NCXMOD_TEMP_DIR (const xmlChar *)"/tmp"

/********************************************************************
*								    *
*			  T Y P E S                                 *
*								    *
*********************************************************************/

/* following 3 structs used for providing temporary 
 * work directories for yangcli sessions
 */

/* program-level temp dir control block */
typedef struct ncxmod_temp_progcb_t_ {
    dlq_hdr_t    qhdr;
    xmlChar     *source;
    dlq_hdr_t    temp_sescbQ;  /* Q of ncxmod_temp_sescb_t */
} ncxmod_temp_progcb_t;


/* session-level temp-dir control block */
typedef struct ncxmod_temp_sescb_t_ {
    dlq_hdr_t    qhdr;
    xmlChar     *source;
    uint32       sidnum;
    dlq_hdr_t    temp_filcbQ;  /* Q of ncxmod_temp_filcb_t */
} ncxmod_temp_sescb_t;


/* temporary file control block */
typedef struct ncxmod_temp_filcb_t_ {
    dlq_hdr_t       qhdr;
    xmlChar        *source;
    const xmlChar  *filename;  /* ptr into source */
} ncxmod_temp_filcb_t;


/* struct for storing YANG file search results
 * this is used by yangcli for schema auto-load
 * also for finding newest version, or all versions
 * within the module path
 */
typedef struct ncxmod_search_result_t_ {
    dlq_hdr_t      qhdr;
    xmlChar       *module;        /* module or submodule name */
    xmlChar       *belongsto;     /* set if submodule & belongs-to found */
    xmlChar       *revision;      /* set if most recent revision found */
    xmlChar       *namespacestr;  /* set if module & namespace found */
    xmlChar       *source;        /* file location */
    ncx_module_t  *mod;           /* back-ptr to found module if loaded */
    status_t       res;           /* search result, only use if NO_ERR */
    uint32         nslen;         /* length of base part of namespacestr */
    cap_rec_t     *cap;           /* back-ptr to source capability URI */
    boolean        capmatch;      /* set by yangcli; internal use only */
    boolean        ismod;         /* TRUE=module; FALSE=submodule */
} ncxmod_search_result_t;


/**********************************************************************
 * ncxmod_callback_fn_t
 *
 * user function callback template to process a module
 * during a subtree traversal
 *
 * Used by the ncxmod_process_subtree function
 *
 * DESCRIPTION:
 *   Handle the current filename in the subtree traversal
 *   Parse the module and generate.
 *
 * INPUTS:
 *   fullspec == absolute or relative path spec, with filename and ext.
 *               this regular file exists, but has not been checked for
 *               read access of 
 *   cookie == opaque handle passed from start of callbacks
 *
 * RETURNS:
 *    status
 *
 *    Return fatal error to stop the traversal or NO_ERR to
 *    keep the traversal going.  Do not return any warning or
 *    recoverable error, just log and move on
 *********************************************************************/
typedef status_t (*ncxmod_callback_fn_t) (const char *fullspec,
					  void *cookie);


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION ncxmod_init
* 
* Initialize the ncxmod module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncxmod_init (void);


/********************************************************************
* FUNCTION ncxmod_cleanup
* 
* Cleanup the ncxmod module
*
*********************************************************************/
extern void
    ncxmod_cleanup (void);


/********************************************************************
* FUNCTION ncxmod_load_module
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
*
* This is the only load module variant that checks if there
* are any errors recorded in the module or any of its dependencies
* !!! ONLY RETURNS TRUE IF MODULE AND ALL IMPORTS ARE LOADED OK !!!
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   savedevQ == Q of ncx_save_deviations_t to use, if any
*   retmod == address of return module (may be NULL)
*
* OUTPUTS:
*   if non-NULL:
*    *retmod == pointer to requested module version
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    ncxmod_load_module (const xmlChar *modname,
			const xmlChar *revision,
                        dlq_hdr_t *savedevQ,
			ncx_module_t **retmod);


/********************************************************************
* FUNCTION ncxmod_parse_module
*
* Determine the location of the specified module
* and then parse the file into an ncx_module_t,
* but do not load it into the module registry
*
* Used by yangcli to build per-session view of each
* module advertised by the NETCONF server
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   savedevQ == Q of ncx_save_deviations_t to use, if any
*   retmod == address of return module
*
* OUTPUTS:
*    *retmod == pointer to requested module version
*               THIS IS A LIVE MALLOCED STRUCT THAT NEEDS
*               FREED LATER
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    ncxmod_parse_module (const xmlChar *modname,
                         const xmlChar *revision,
                         dlq_hdr_t *savedevQ,
                         ncx_module_t **retmod);


/********************************************************************
* FUNCTION ncxmod_find_module
*
* Determine the location of the specified module
* and then fill out a search result struct
* DO NOT load it into the system
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*
* RETURNS:
*   == malloced and filled in search result struct
*      MUST call ncxmod_free_search_result() if the
*      return value is non-NULL
*      CHECK result->res TO SEE IF MODULE WAS FOUND
*      A SEARCH RESULT WILL BE RETURNED IF A SEARCH WAS
*      ATTEMPTED, EVEN IF NOTHING FOUND
*   == NULL if any error preventing a search
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_find_module (const xmlChar *modname,
			const xmlChar *revision);


/********************************************************************
* FUNCTION ncxmod_find_all_modules
*
* Determine the location of all possible YANG modules and submodules
* within the configured YUMA_MODPATH and default search path
* All files with .yang and .yin file extensions found in the
* search directories will be checked.
* 
* Does not cause modules to be fully parsed and registered.
* Quick parse only is done, and modules are discarded.
* Strings from the module are copied into the searchresult struct
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) HOME/modules directory
* 3) YUMA_HOME/modules directory
* 4) YUMA_INSTALL/modules directory
*
* INPUTS:
*   resultQ == address of Q to stor malloced search results
*
* OUTPUTS:
*  resultQ may have malloced ncxmod_zsearch_result_t structs
*  queued into it representing the modules found in the search
*
* RETURNS:
*  status
*********************************************************************/
extern status_t 
    ncxmod_find_all_modules (dlq_hdr_t *resultQ);


/********************************************************************
* FUNCTION ncxmod_load_deviation
*
* Determine the location of the specified module
* and then parse it in the special deviation mode
* and save any deviation statements in the Q that is provided
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   devname == deviation module name with 
*                   no path prefix or file extension
*   deviationQ == address of Q of ncx_save_deviations_t  structs
*                 to add any new entries 
*
* OUTPUTS:
*   if non-NULL:
*    deviationQ has malloced ncx_save_deviations_t structs added
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    ncxmod_load_deviation (const xmlChar *deviname,
                           dlq_hdr_t *deviationQ);


/********************************************************************
* FUNCTION ncxmod_load_imodule
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
*
* Called from an include or import or submodule
* Includes the YANG parser control block and new parser source type
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   pcb == YANG parser control block
*   ptyp == YANG parser source type
*   parent == pointer to module being parsed if this is a
*     a request to parse a submodule; there is only 1 parent for 
*     all submodules, based on the value of belongs-to
*   retmod == address of return module
* OUTPUTS:
*  *retmod == pointer to found module (if NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    ncxmod_load_imodule (const xmlChar *modname,
			 const xmlChar *revision,
			 yang_pcb_t *pcb,
			 yang_parsetype_t ptyp,
                         ncx_module_t *parent,
                         ncx_module_t **retmod);


/********************************************************************
* FUNCTION ncxmod_load_module_ex
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
* Return the PCB instead of deleting it
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   with_submods == TRUE if YANG_PT_TOP mode should skip submodules
*                == FALSE if top-level mode should process sub-modules 
*   savetkc == TRUE if the parse chain should be saved (e.g., YIN)
*   keepmode == TRUE if pcb->top should be saved even if it
*               is already loaded;  FALSE will allow the mod
*               to be swapped out and the new copy deleted
*               yangdump sets this to true
*   docmode == TRUE if need to preserve strings for --format=html or yang
*           == FALSE if no need to preserve string token sequences
*   savedevQ == Q of ncx_save_deviations_t to use
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   pointer to malloced parser control block, or NULL of none
*********************************************************************/
extern yang_pcb_t *
    ncxmod_load_module_ex (const xmlChar *modname,
                           const xmlChar *revision,
                           boolean with_submods,
                           boolean savetkc,
                           boolean keepmode,
                           boolean docmode,
                           dlq_hdr_t *savedevQ,
                           status_t  *res);


/********************************************************************
* FUNCTION ncxmod_load_module_diff
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
* Return the PCB instead of deleting it
* !!Do not add definitions to the registry!!
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   with_submods == TRUE if YANG_PT_TOP mode should skip submodules
*                == FALSE if top-level mode should process sub-modules 
*   modpath == module path to override the modpath CLI var or
*              the YUMA_MODPATH env var
*   savedevQ == Q of ncx_save_deviations_t to use
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   pointer to malloced parser control block, or NULL of none
*********************************************************************/
extern yang_pcb_t *
    ncxmod_load_module_diff (const xmlChar *modname,
			     const xmlChar *revision,
			     boolean with_submods,
			     const xmlChar *modpath,
                             dlq_hdr_t  *savedevQ,
			     status_t  *res);


/********************************************************************
* FUNCTION ncxmod_find_data_file
*
* Determine the location of the specified data file
*
* Search order:
*
* 1) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 2) current directory or absolute path
* 3) HOME/data directory
* 4) YUMA_HOME/data directory
* 5) HOME/.yuma/ directory
* 6a) YUMA_INSTALL/data directory OR
* 6b) /usr/share/yuma/data directory
* 7) /etc/yuma directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   generrors == TRUE if error message should be generated
*                FALSE if no error message
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
extern xmlChar *
    ncxmod_find_data_file (const xmlChar *fname,
			   boolean generrors,
			   status_t *res);


/********************************************************************
* FUNCTION ncxmod_find_sil_file
*
* Determine the location of the specified 
* server instrumentation library file
*
* Search order:
*
*  1) $YUMA_HOME/target/lib directory 
*  2) $YUMA_RUNPATH environment variable
*  3) $YUMA_INSTALL/lib
*  4) $YUMA_INSTALL/lib/yuma
*  5) /usr/lib64/yuma directory (LIB64 only)
*  5 or 6) /usr/lib/yuma directory
*
* INPUTS:
*   fname == SIL file name with extension
*   generrors == TRUE if error message should be generated
*                FALSE if no error message
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
extern xmlChar *
    ncxmod_find_sil_file (const xmlChar *fname,
                          boolean generrors,
                          status_t *res);


/********************************************************************
* FUNCTION ncxmod_make_data_filespec
*
* Determine a suitable path location for the specified data file name
*
* Search order:
*
* 1) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 2) HOME/data directory
* 3) YUMA_HOME/data directory
* 4) HOME/.yuma directory
* 5) YUMA_INSTALL/data directory
* 6) current directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if no suitable location
*   for the datafile is found
*   It must be freed after use!!!
*********************************************************************/
extern xmlChar *
    ncxmod_make_data_filespec (const xmlChar *fname,
                               status_t *res);


/********************************************************************
* FUNCTION ncxmod_make_data_filespec_from_src
*
* Determine the directory path portion of the specified
* source_url and change the filename to the specified filename
* in a new copy of the complete filespec
*
* INPUTS:
*   srcspec == source filespec to use
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if some error occurred
*********************************************************************/
extern xmlChar *
    ncxmod_make_data_filespec_from_src (const xmlChar *srcspec,
                                        const xmlChar *fname,
                                        status_t *res);


/********************************************************************
* FUNCTION ncxmod_find_script_file
*
* Determine the location of the specified script file
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_RUNPATH environment var (or set by runpath CLI var)
* 3) HOME/scripts directory
* 4) YUMA_HOME/scripts directory
* 5) YUMA_INSTALL/scripts directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
extern xmlChar *
    ncxmod_find_script_file (const xmlChar *fname,
			     status_t *res);


/********************************************************************
* FUNCTION ncxmod_set_home
* 
*   Override the HOME env var with the home CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   home == new HOME value
*        == NULL or empty string to disable
*********************************************************************/
extern void
    ncxmod_set_home (const xmlChar *home);


/********************************************************************
* FUNCTION ncxmod_get_home
*
*  Get the HOME or --home parameter value,
*  whichever is in effect, if any
*
* RETURNS:
*   const point to the home variable, or NULL if not set
*********************************************************************/
extern const xmlChar *
    ncxmod_get_home (void);


/********************************************************************
* FUNCTION ncxmod_set_yuma_home
* 
*   Override the YUMA_HOME env var with the yuma-home CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   yumahome == new YUMA_HOME value
*            == NULL or empty string to disable
*********************************************************************/
extern void
    ncxmod_set_yuma_home (const xmlChar *yumahome);


/********************************************************************
* FUNCTION ncxmod_get_yuma_home
*
*  Get the YUMA_HOME or --yuma-home parameter value,
*  whichever is in effect, if any
*
* RETURNS:
*   const point to the yuma_home variable, or NULL if not set
*********************************************************************/
extern const xmlChar *
    ncxmod_get_yuma_home (void);


/********************************************************************
* FUNCTION ncxmod_get_yuma_install
*
*  Get the YUMA_INSTALL or default install parameter value,
*  whichever is in effect
*
* RETURNS:
*   const point to the YUMA_INSTALL value
*********************************************************************/
extern const xmlChar *
    ncxmod_get_yuma_install (void);


/********************************************************************
* FUNCTION ncxmod_set_modpath
* 
*   Override the YUMA_MODPATH env var with the modpath CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   modpath == new YUMA_MODPATH value
*           == NULL or empty string to disable
*********************************************************************/
extern void
    ncxmod_set_modpath (const xmlChar *modpath);


/********************************************************************
* FUNCTION ncxmod_set_datapath
* 
*   Override the YUMA_DATAPATH env var with the datapath CLI var
*
* INPUTS:
*   datapath == new YUMA_DATAPATH value
*           == NULL or empty string to disable
*
*********************************************************************/
extern void
    ncxmod_set_datapath (const xmlChar *datapath);


/********************************************************************
* FUNCTION ncxmod_set_runpath
* 
*   Override the YUMA_RUNPATH env var with the runpath CLI var
*
* INPUTS:
*   datapath == new YUMA_RUNPATH value
*           == NULL or empty string to disable
*
*********************************************************************/
extern void
    ncxmod_set_runpath (const xmlChar *runpath);


/********************************************************************
* FUNCTION ncxmod_set_subdirs
* 
*   Set the subdirs flag to FALSE if the no-subdirs CLI param is set
*
* INPUTS:
*  usesubdirs == TRUE if subdirs searchs should be done
*             == FALSE if subdir searches should not be done
*********************************************************************/
extern void
    ncxmod_set_subdirs (boolean usesubdirs);


/********************************************************************
* FUNCTION ncxmod_get_yumadir
* 
*   Get the yuma directory being used
*
* RETURNS:
*   pointer to the yuma dir string
*********************************************************************/
extern const xmlChar *
    ncxmod_get_yumadir (void);


/********************************************************************
* FUNCTION ncxmod_process_subtree
*
* Search the entire specified subtree, looking for YANG
* modules.  Invoke the callback function for each module
* file found
*
* INPUTS:
*    startspec == absolute or relative pathspec to start
*            the search.  If this is not a valid pathname,
*            processing will exit immediately.
*    callback == address of the ncxmod_callback_fn_t function
*         to use for this traveral
*    cookie == cookie to pass to each invocation of the callback
*
* OUTPUTS:
*   *done == TRUE if done processing
*            FALSE to keep going
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
extern status_t
    ncxmod_process_subtree (const char *startspec, 
			    ncxmod_callback_fn_t callback,
			    void *cookie);


/********************************************************************
* FUNCTION ncxmod_test_subdir
*
* Check if the specified string is a directory
*
* INPUTS:
*    dirspec == string to check as a directory spec
*
* RETURNS:
*    TRUE if the string is a directory spec that this user
*       is allowed to open
*    FALSE otherwise
*********************************************************************/
extern boolean
    ncxmod_test_subdir (const xmlChar *dirspec);


/********************************************************************
* FUNCTION ncxmod_get_userhome
*
* Get the user home dir from the passwd file
*
* INPUTS:
*    user == user name string (may not be zero-terminiated)
*    userlen == length of user
*
* RETURNS:
*    const pointer to the user home directory string
*********************************************************************/
extern const xmlChar *
    ncxmod_get_userhome (const xmlChar *user,
			 uint32 userlen);


/********************************************************************
* FUNCTION ncxmod_get_envvar
*
* Get the specified shell environment variable
*
* INPUTS:
*    name == name of the environment variable (may not be zero-terminiated)
*    namelen == length of name string
*
* RETURNS:
*    const pointer to the specified environment variable value
*********************************************************************/
extern const xmlChar *
    ncxmod_get_envvar (const xmlChar *name,
		       uint32 namelen);


/********************************************************************
* FUNCTION ncxmod_set_altpath
*
* Set the alternate path that should be used first (for yangdiff)
*
* INPUTS:
*    altpath == full path string to use
*               must be static 
*               a const back-pointer is kept, not a copy
*
*********************************************************************/
extern void
    ncxmod_set_altpath (const xmlChar *altpath);


/********************************************************************
* FUNCTION ncxmod_clear_altpath
*
* Clear the alternate path so none is used (for yangdiff)
*
*********************************************************************/
extern void
    ncxmod_clear_altpath (void);


/********************************************************************
* FUNCTION ncxmod_list_data_files
*
* List the available data files found in the data search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 3) HOME/data directory
* 4) YUMA_HOME/data directory
* 5) YUMA_INSTALL/data directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    ncxmod_list_data_files (help_mode_t helpmode,
                            boolean logstdout);


/********************************************************************
* FUNCTION ncxmod_list_script_files
*
* List the available script files found in the 'run' search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_RUNPATH environment var (or set by datapath CLI var)
* 3) HOME/scripts directory
* 4) YUMA_HOME/scripts directory
* 5) YUMA_INSTALL/scripts directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    ncxmod_list_script_files (help_mode_t helpmode,
                              boolean logstdout);


/********************************************************************
* FUNCTION ncxmod_list_yang_files
*
* List the available YANG files found in the 'mod' search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_MODPATH environment var (or set by datapath CLI var)
* 3) HOME/modules directory
* 4) YUMA_HOME/modules directory
* 5) YUMA_INSTALL/modules directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    ncxmod_list_yang_files (help_mode_t helpmode,
                            boolean logstdout);


/********************************************************************
* FUNCTION ncxmod_setup_yumadir
*
* Setup the ~/.yuma directory if it does not exist
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncxmod_setup_yumadir (void);


/********************************************************************
* FUNCTION ncxmod_setup_tempdir
*
* Setup the ~/.yuma/tmp directory if it does not exist
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncxmod_setup_tempdir (void);



/********************************************************************
* FUNCTION ncxmod_new_program_tempdir
*
*   Setup a program instance temp files directory
*
* INPUTS:
*   res == address of return status
*
* OUTPUTS:
*  *res  == return status
*
* RETURNS:
*   malloced tempdir_progcb_t record (if NO_ERR)
*********************************************************************/
extern ncxmod_temp_progcb_t *
    ncxmod_new_program_tempdir (status_t *res);


/********************************************************************
* FUNCTION ncxmod_free_program_tempdir
*
*   Remove a program instance temp files directory
*
* INPUTS:
*   progcb == tempoeray program instance control block to free
*
*********************************************************************/
extern void
    ncxmod_free_program_tempdir (ncxmod_temp_progcb_t *progcb);


/********************************************************************
* FUNCTION ncxmod_new_session_tempdir
*
*   Setup a session instance temp files directory
*
* INPUTS:
*    progcb == program instance control block to use
*    sid == manager session ID of the new session instance control block
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced tempdir_sescb_t record
*********************************************************************/
extern ncxmod_temp_sescb_t *
    ncxmod_new_session_tempdir (ncxmod_temp_progcb_t *progcb,
                                uint32 sidnum,
                                status_t *res);


/********************************************************************
* FUNCTION ncxmod_free_session_tempdir
*
*   Clean and free a session instance temp files directory
*
* INPUTS:
*    progcb == program instance control block to use
*    sidnum == manager session number to delete
*
*********************************************************************/
extern void
    ncxmod_free_session_tempdir (ncxmod_temp_progcb_t *progcb,
                                 uint32 sidnum);


/********************************************************************
* FUNCTION ncxmod_new_session_tempfile
*
*   Setup a session instance temp file for writing
*
* INPUTS:
*    sescb == session instance control block to use
*    filename == filename to create in the temp directory
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced tempdir_sescb_t record
*********************************************************************/
extern ncxmod_temp_filcb_t *
    ncxmod_new_session_tempfile (ncxmod_temp_sescb_t *sescb,
                                 const xmlChar *filename,
                                 status_t *res);


/********************************************************************
* FUNCTION ncxmod_free_session_tempfile
*
*   Clean and free a session instance temp files directory
*
* INPUTS:
*    filcb == file control block to delete
*
*********************************************************************/
extern void
    ncxmod_free_session_tempfile (ncxmod_temp_filcb_t *filcb);


/********************************************************************
* FUNCTION ncxmod_new_search_result
*
*  Malloc and initialize a search result struct
*
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_new_search_result (void);


/********************************************************************
* FUNCTION ncxmod_new_search_result_ex
*
*  Malloc and initialize a search result struct
*
* INPUTS:
*    mod == module struct to use
*
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_new_search_result_ex (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncxmod_new_search_result_str
*
*  Malloc and initialize a search result struct
*
* INPUTS:
*    modname == module name string to use
*    revision == revision date to use (may be NULL)
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_new_search_result_str (const xmlChar *modname,
                                  const xmlChar *revision);


/********************************************************************
* FUNCTION ncxmod_free_search_result
*
*  Clean and free a search result struct
*
* INPUTS:
*    searchresult == struct to clean and free
*********************************************************************/
extern void
    ncxmod_free_search_result (ncxmod_search_result_t *searchresult);



/********************************************************************
* FUNCTION ncxmod_clean_search_result_queue
*
*  Clean and free all the search result structs
*  in the specified Q
*
* INPUTS:
*    searchQ = Q of ncxmod_search_result_t to clean and free
*********************************************************************/
extern void
    ncxmod_clean_search_result_queue (dlq_hdr_t *searchQ);


/********************************************************************
* FUNCTION ncxmod_find_search_result
*
*  Find a search result inthe specified Q
*
* Either modname or nsuri must be set
* If modname is set, then revision will be checked
*
* INPUTS:
*    searchQ = Q of ncxmod_search_result_t to check
*    modname == module or submodule name to find
*    revision == revision-date to find
*    nsuri == namespace URI fo find
* RETURNS:
*   pointer to first matching record; NULL if not found
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_find_search_result (dlq_hdr_t *searchQ,
                               const xmlChar *modname,
                               const xmlChar *revision,
                               const xmlChar *nsuri);


/********************************************************************
* FUNCTION ncxmod_clone_search_result
*
*  Clone a search result
*
* INPUTS:
*    sr = searchresult to clone
*
* RETURNS:
*   pointer to malloced and filled in clone of sr
*********************************************************************/
extern ncxmod_search_result_t *
    ncxmod_clone_search_result (const ncxmod_search_result_t *sr);


/********************************************************************
* FUNCTION ncxmod_test_filespec
*
* Check the exact filespec to see if it a file
*
* INPUTS:
*    filespec == file spec to check
*
* RETURNS:
*    TRUE if valid readable file
*    FALSE otherwise
*********************************************************************/
extern boolean
    ncxmod_test_filespec (const xmlChar *filespec);


/********************************************************************
* FUNCTION ncxmod_get_pathlen_from_filespec
*
* Get the length of th path part of the filespec string
*
* INPUTS:
*    filespec == file spec to check
*
* RETURNS:
*    number of chars to keep for the path spec
*********************************************************************/
extern uint32
    ncxmod_get_pathlen_from_filespec (const xmlChar *filespec);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncxmod */
