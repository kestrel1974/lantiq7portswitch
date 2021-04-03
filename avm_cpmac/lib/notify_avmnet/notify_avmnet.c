#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "notify_avmnet.h"

#define DEBUG           1
#define LOG_TO_FILE     1
#define LOGFILE "/var/log/notify_avmnet.log"
#define CONSOLE "/dev/console"

#define ERROR             -1
#define MODULE_BUFFER_SIZE 1024
#define DEVNAME_MAX_LEN    64
#define PARAM_STR_LEN 128
#define MAX_MOD_PARAMS 32
char *ethwan_default = "ethwan=2";
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

enum reboot_request {
	DONT_REBOOT_IF_MODULE_PRESENT = 0,
	REBOOT_IF_MODULE_PRESENT      = 1
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

enum system_state {
	NO_REBOOT = 0,
	REBOOT    = 1
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

enum module_state {
	MODULE_NOT_PRESENT  = 0,
	MODULE_PRESENT      = 1,
	MODULE_CHECK_FAILED = 2
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void dbg_to_logfile(const char *logfile, const char *t,...){
	FILE *log_fp;
	log_fp = fopen(logfile, "a");
	if (log_fp != NULL){
		va_list ap;
		va_start(ap, t);
		vfprintf(log_fp, t, ap);
		va_end(ap);
		fclose(log_fp);
	}
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#define CONSOLE_PRINT(format, arg...) dbg_to_logfile( CONSOLE, format, ##arg );

#if defined(DEBUG) && DEBUG
#if defined(LOG_TO_FILE) && LOG_TO_FILE
#define DBG_PRINT(format, arg...)   dbg_to_logfile(LOGFILE, "[%s/%d]: " format "\n", __func__, __LINE__, ##arg);
#else  //#if defined(LOG_TO_FILE)
#define DBG_PRINT(format, arg...)   printf("[%s/%d]: " format "\n", __func__, __LINE__, ##arg);
#endif //#if defined(LOG_TO_FILE)
#else //#if defined(DEBUG)
#define DBG_PRINT(format, arg...)
#endif //#if defined(DEBUG)
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static char module_buffer[MODULE_BUFFER_SIZE];

#if defined(CONFIG_VR9)
#define CPU "vr9"
#define HW_NEEDS_REBOOT_IF_MP DONT_REBOOT_IF_MODULE_PRESENT
#endif

#if defined(CONFIG_AR9)
#define CPU "ar9"
#define HW_NEEDS_REBOOT_IF_MP DONT_REBOOT_IF_MODULE_PRESENT
#endif

#if defined(CONFIG_AR10)
#define CPU "ar10"
#define HW_NEEDS_REBOOT_IF_MP DONT_REBOOT_IF_MODULE_PRESENT
#endif

#ifndef CPU
#error CPU has to be AR9, AR10 or VR9
#endif

static char *all_modules_list[] = {
	"ifx_ppa_mini_qos",
	"ifx_ppa_mini_sessions",
	"ifxmips_ppa_hal_" CPU "_e5",
	"ifxmips_ppa_datapath_" CPU "_e5",
	"ifxmips_ppa_hal_" CPU "_a5",
	"ifxmips_ppa_datapath_" CPU "_a5"};
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


static enum module_state check_module_present( const char *module_name ){

	int read_len = 0;
	int fd = open("/proc/modules", O_RDONLY);
	if (fd < 0)
		return MODULE_CHECK_FAILED;

	while ( read_len = read(fd, module_buffer, MODULE_BUFFER_SIZE - 1) ) {
		if (read_len < 0 )
			break;
		module_buffer[read_len] = 0;

		if( strstr(module_buffer, module_name) ){
			close(fd);
			return MODULE_PRESENT;
		}
	}
	close(fd);
	return MODULE_NOT_PRESENT;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void exec_subproc_block(const char *path, char *const argv[]){
	pid_t pid;
	int status;
	pid_t ret;
	int i = 0;

	DBG_PRINT("run cmd '%s' ", path );
	while(argv[i]){
		DBG_PRINT("  argv[%d]='%s'",i, argv[i]);
		i++;
	}

	pid = fork();
	if (pid == -1) {
		DBG_PRINT("cannot fork: %d", errno);
		return;
	} else if (pid != 0) {
		while ((ret = waitpid(pid, &status, 0)) == -1) {
			if (errno != EINTR) {
				DBG_PRINT("waitpid error: %d", errno);
				break;
			}
		}
	} else {
		if (execve(path, argv, NULL) == -1) {
			DBG_PRINT("execve error: %d", errno);
			return;
		}
	}

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void modprobe_once( char *module_name, size_t paramc, char **paramv ){

	if ( check_module_present( module_name ) == MODULE_NOT_PRESENT ){

		if (paramc == 0) {
			char *modprobe_argv[] = {"modprobe", module_name, NULL};
			exec_subproc_block("/sbin/modprobe", modprobe_argv);
		}
		else if (paramc<MAX_MOD_PARAMS) {
			size_t i;
			char *modprobe_argv[MAX_MOD_PARAMS+3] = {"modprobe", module_name, NULL};
			for (i=0; i<paramc; i++){
				modprobe_argv[i+2] = paramv[i];
				modprobe_argv[i+3] = NULL;
			}
			exec_subproc_block("/sbin/modprobe", modprobe_argv);
		}
		else {
			DBG_PRINT("too many params for modprobe_once %d/%d\n", paramc, MAX_MOD_PARAMS);
		}
	}
}



/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static enum system_state unload_all_ppa_modules( enum reboot_request reboot_if_module_present ) {
	int i;

	DBG_PRINT("called");
	for ( i = 0; i < sizeof(all_modules_list) / sizeof(all_modules_list[0]); i++){
		if ( check_module_present( all_modules_list[i] ) == MODULE_PRESENT) {

			DBG_PRINT("%s is loaded", all_modules_list[i] );

			if ( reboot_if_module_present == REBOOT_IF_MODULE_PRESENT ){
				DBG_PRINT("PPA Module %s present -> we need to reboot the box", all_modules_list[i]);
				CONSOLE_PRINT("\nAVMNET:PPA Module %s present -> we need to reboot the box\n", all_modules_list[i]);
				system( "reboot" );
				return REBOOT;
			}
			else {

				char *const rmmod_argv[] = {"rmmod", all_modules_list[i], NULL};
				exec_subproc_block("/sbin/rmmod", rmmod_argv );

				if ( check_module_present( all_modules_list[i] ) == MODULE_PRESENT ) {
					DBG_PRINT("ERROR: unload '%s' did not work", all_modules_list[i] );
					CONSOLE_PRINT("ERROR: unload '%s' did not work", all_modules_list[i] );
					CONSOLE_PRINT("\nAVMNET:PPA Module %s present -> we need to reboot the box\n", all_modules_list[i]);
					system( "reboot" );
				}
			}
		} else {
			DBG_PRINT("%s not loaded", all_modules_list[i] );
		}
	}
	return NO_REBOOT;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int kernel_read_current_ata_dev(char *dst) {
	int read_len = 0;
	int fd;

	if (!dst)
		return ERROR;

	fd = open("/proc/eth/avm_ata_dev", O_RDONLY);
	if (fd < 0){
		DBG_PRINT("cannot open /proc/eth/avm_ata_dev");
		dst[0] = 0;
		return ERROR;
	}

	read_len = read(fd, dst, DEVNAME_MAX_LEN - 1) ;
	if (read_len > 0 ){
		dst[read_len] = 0;
	} else {
		dst[0] = 0;
	}
	close(fd);
	DBG_PRINT("current_ata_dev: %s", dst );
	return read_len;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int kernel_read_current_wan_mode() {
	int read_len = 0;
	int fd = open("/proc/eth/avm_wan_mode", O_RDONLY);
	if (fd < 0){
		DBG_PRINT("cannot open /proc/eth/avm_wan_mode");
		return ERROR;
	}

	while ( read_len = read(fd, module_buffer, MODULE_BUFFER_SIZE - 1) ) {
		if (read_len < 0 )
			break;
		module_buffer[read_len] = 0;

		if( strstr(module_buffer, "ATM") ){
			close(fd);
			return TRANSFER_MODE_ATM;
		}
		if( strstr(module_buffer, "PTM") ){
			close(fd);
			return TRANSFER_MODE_PTM;
		}
		if( strstr(module_buffer, "ETH") ){
			close(fd);
			return TRANSFER_MODE_ETH;
		}
		if( strstr(module_buffer, "KDEV") ){
			close(fd);
			return TRANSFER_MODE_KDEV;
		}
	}
	close(fd);

	return TRANSFER_MODE_NONE;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void notify_avmnet_hw_start_selected_tm_generic( uint32_t new_transfer_mode, uint32_t broadband_type, uint32_t no_dsl_init )
{
	char no_dsl_init_str[PARAM_STR_LEN];
	int current_transfer_mode;
	char *paramv[MAX_MOD_PARAMS];
	size_t paramc = 0;

	DBG_PRINT("called, avm_dsl_no_init=%lu",(unsigned long)no_dsl_init );

	if ( no_dsl_init ) {
#if(CONFIG_VR9)
		snprintf( no_dsl_init_str, PARAM_STR_LEN - 1, "avm_dsl_no_init=%lu", (unsigned long)no_dsl_init );
		paramv[paramc++] = no_dsl_init_str;
#else
		DBG_PRINT("avm_dsl_no_init not supported on this platform!" );
#endif
	}
	if (broadband_type != BROADBAND_TYPE_DSL){
		DBG_PRINT("ignoring unknown broadband type: %d", broadband_type);
		return;
	}

	current_transfer_mode = kernel_read_current_wan_mode();

	if ( new_transfer_mode ==  current_transfer_mode ){

		DBG_PRINT("Tranfermode: %d already set", current_transfer_mode);
		return;
	}

	if ( new_transfer_mode == TRANSFER_MODE_ATM ){

		DBG_PRINT("Start Training ATM");

		if ( unload_all_ppa_modules( HW_NEEDS_REBOOT_IF_MP ) == REBOOT)
			return;

		modprobe_once("ifxmips_ppa_datapath_" CPU "_a5", paramc, paramv);

	} else if( new_transfer_mode == TRANSFER_MODE_PTM ) {

		DBG_PRINT("Start Training PTM");
		if ( unload_all_ppa_modules( HW_NEEDS_REBOOT_IF_MP ) == REBOOT)
			return;

		modprobe_once("ifxmips_ppa_datapath_" CPU "_e5", paramc, paramv);

	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void notify_avmnet_hw_start_selected_tm( uint32_t new_transfer_mode, uint32_t broadband_type ){
	notify_avmnet_hw_start_selected_tm_generic( new_transfer_mode, broadband_type, 0);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void notify_avmnet_hw_start_selected_tm_no_dsl_init( uint32_t new_transfer_mode, uint32_t broadband_type ){
	notify_avmnet_hw_start_selected_tm_generic( new_transfer_mode, broadband_type, 1 );
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void notify_avmnet_hw_link_established( uint32_t transfer_mode, uint32_t broadband_type ) {

	DBG_PRINT("called");
	if (broadband_type != BROADBAND_TYPE_DSL){
		DBG_PRINT("ignoring unknown broadband type: %d", broadband_type);
		return;
	}

	if ( transfer_mode == TRANSFER_MODE_ATM ){

		DBG_PRINT("Showtime ATM");
		modprobe_once("ifxmips_ppa_hal_" CPU "_a5", 0, NULL);

	} else if( transfer_mode == TRANSFER_MODE_PTM ) {

		DBG_PRINT("Showtime PTM");
		modprobe_once("ifxmips_ppa_hal_" CPU "_e5", 0, NULL);

	}

	if (( transfer_mode == TRANSFER_MODE_ATM ) || ( transfer_mode == TRANSFER_MODE_PTM ) ){
		modprobe_once("ifx_ppa_mini_sessions", 0, NULL);
		modprobe_once("ifx_ppa_mini_qos", 0, NULL);
	}
}

/*------------------------------------------------------------------------------------------*\
 * depricated, is replaced by notify_avmnet_hw_setup_ata_dev
\*------------------------------------------------------------------------------------------*/
void notify_avmnet_hw_disable_phy( uint32_t broadband_type )
{

#if defined(USE_HW_DISABLE_PHY) && USE_HW_DISABLE_PHY
	int current_transfer_mode;
	char *paramv[MAX_MOD_PARAMS];
	size_t paramc = 0;

	if (broadband_type != BROADBAND_TYPE_DSL){
		DBG_PRINT("ignoring unknown broadband type: %d", broadband_type);
		return;
	}
	current_transfer_mode = kernel_read_current_wan_mode();
	if ( (current_transfer_mode == TRANSFER_MODE_ETH ) || (current_transfer_mode == TRANSFER_MODE_KDEV ) ) {
		DBG_PRINT("ATA-Mode already set");
		return;
	}

	DBG_PRINT("starting AVM ATA Mode");
	if ( unload_all_ppa_modules( HW_NEEDS_REBOOT_IF_MP ) == REBOOT)
		return;

	paramv[paramc++] = ethwan_default;
#if defined(CONFIG_VR9)
	//ethwan=2 --> EthernetWan
	modprobe_once("ifxmips_ppa_datapath_" CPU "_e5", paramc, paramv );
	modprobe_once("ifxmips_ppa_hal_" CPU "_e5", 0, NULL);
#else
	//ethwan=2 --> EthernetWan
	modprobe_once("ifxmips_ppa_datapath_" CPU "_a5", paramc, paramv );
	modprobe_once("ifxmips_ppa_hal_" CPU "_a5",0 , NULL);
#endif
	modprobe_once("ifx_ppa_mini_sessions", 0, NULL);
	modprobe_once("ifx_ppa_mini_qos", 0, NULL);
#else //#if defined(USE_HW_DISABLE_PHY) && USE_HW_DISABLE_PHY

	DBG_PRINT("called, but obsolete - use notify_avmnet_hw_setup_wan_dev instead");

#endif //#if defined(USE_HW_DISABLE_PHY) && USE_HW_DISABLE_PHY
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void notify_avmnet_hw_setup_ata_kdev( char *wan_dev_name ){

	DBG_PRINT("setup_ata_kdev - just keep old state");
	return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void notify_avmnet_hw_setup_ata_eth_dev( char *wan_dev_name )
{
	char ata_param_str[PARAM_STR_LEN];
	int current_transfer_mode;
	char *paramv[MAX_MOD_PARAMS];
	size_t paramc = 0;


	DBG_PRINT("called, wan_dev=%s", wan_dev_name?wan_dev_name:"NO_WAN_DEV");

	current_transfer_mode = kernel_read_current_wan_mode();

	if (  current_transfer_mode == TRANSFER_MODE_ETH ) {
		char current_ata_dev[DEVNAME_MAX_LEN];
		DBG_PRINT("ATA-Mode already set");

		kernel_read_current_ata_dev(current_ata_dev);

		if (wan_dev_name == NULL) {
			if ( strncmp( current_ata_dev,"NONE", DEVNAME_MAX_LEN) == 0) {
				DBG_PRINT("no wan dev in past, no wan dev in future");
				return;
			}
		} else {
			if ( strncmp( current_ata_dev, wan_dev_name , DEVNAME_MAX_LEN) == 0 ){
				DBG_PRINT("keep module state, ata dev %s was already set", wan_dev_name);
				return;
			}
		}
	}

	if ( unload_all_ppa_modules( HW_NEEDS_REBOOT_IF_MP ) == REBOOT )
		return;

	paramv[paramc++]=ethwan_default;
	if ( wan_dev_name ){
		snprintf( ata_param_str, PARAM_STR_LEN - 1, "avm_ata_dev=%s", wan_dev_name );
		paramv[paramc++]=ata_param_str;
	}

	DBG_PRINT("starting AVM ATA Mode %s",wan_dev_name?ata_param_str:"");

#if defined(CONFIG_AR9) || defined(CONFIG_AR10)
	//ethwan=2 --> EthernetWan
	modprobe_once("ifxmips_ppa_datapath_" CPU "_a5" , paramc, paramv);
	modprobe_once("ifxmips_ppa_hal_" CPU "_a5", 0, NULL);
#else
	//ethwan=2 --> EthernetWan
	modprobe_once("ifxmips_ppa_datapath_" CPU "_e5" , paramc, paramv);
	modprobe_once("ifxmips_ppa_hal_" CPU "_e5", 0, NULL);
#endif
	modprobe_once("ifx_ppa_mini_sessions", 0, NULL);
	modprobe_once("ifx_ppa_mini_qos", 0, NULL);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void notify_avmnet_hw_exit( uint32_t broadband_type ){

	DBG_PRINT("called");
	if (broadband_type != BROADBAND_TYPE_DSL){
		DBG_PRINT("ignoring unknown broadband type: %d", broadband_type);
		return;
	}

	DBG_PRINT("Unloading all PPA Modules");
	unload_all_ppa_modules( DONT_REBOOT_IF_MODULE_PRESENT );

}

