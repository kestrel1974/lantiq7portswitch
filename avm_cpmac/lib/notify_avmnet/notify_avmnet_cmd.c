#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

#include "notify_avmnet.h"


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void print_usage(){

	printf("USAGE: notify_avmnet { --broadband_type DSL/ATA }  [ hw_start_tm_atm | \n"
	       "                                                    hw_start_tm_atm_no_dsl_init | \n"
	       "                                                    hw_start_tm_ptm | \n"
	       "                                                    hw_link_established_atm | \n"
	       "                                                    hw_link_established_ptm | \n"
	       "                                                    hw_disable_phy | \n"
	       "                                                    hw_setup_ata_eth_dev { devname } | \n"
	       "                                                    hw_setup_ata_kdev { devname } | \n"
	       "                                                    hw_exit ] \n");
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int main(int argc, char **argv) {

	uint32_t broadband_type = BROADBAND_TYPE_DSL;
	int argpos = 1;

	void (*hw_start_selected_tm)( uint32_t transfer_mode, uint32_t broadband_type );
	void (*hw_start_selected_tm_no_dsl_init)( uint32_t transfer_mode, uint32_t broadband_type );
	void (*hw_link_established)( uint32_t transfer_mode, uint32_t broadband_type );
	void (*hw_disable_phy)( uint32_t broadband_type );
	void (*hw_exit)( uint32_t broadband_type );
	void (*hw_setup_ata_kdev)( char *wan_dev_name );
	void (*hw_setup_ata_eth_dev)( char *wan_dev_name );
	void *dl_handle;

	if ( argc < 2 ){
		print_usage();
		return EXIT_FAILURE;
	}

	dl_handle = dlopen("libnotify_avmnet.so", RTLD_NOW | RTLD_GLOBAL );
	if ( !dl_handle ){
		printf("dlopen did not work\n");
		return EXIT_FAILURE;
	}

	hw_start_selected_tm    = dlsym( dl_handle, "notify_avmnet_hw_start_selected_tm" );
	hw_start_selected_tm_no_dsl_init    = dlsym( dl_handle, "notify_avmnet_hw_start_selected_tm_no_dsl_init" );
	hw_link_established     = dlsym( dl_handle, "notify_avmnet_hw_link_established" );
	hw_disable_phy          = dlsym( dl_handle, "notify_avmnet_hw_disable_phy" );
	hw_setup_ata_eth_dev    = dlsym( dl_handle, "notify_avmnet_hw_setup_ata_eth_dev" );
	hw_setup_ata_kdev       = dlsym( dl_handle, "notify_avmnet_hw_setup_ata_kdev" );
	hw_exit          		= dlsym( dl_handle, "notify_avmnet_hw_exit" );

	if ( ! ( hw_start_selected_tm
		 && hw_start_selected_tm_no_dsl_init
		 && hw_link_established
		 && hw_disable_phy
		 && hw_setup_ata_eth_dev
		 && hw_setup_ata_kdev
		 && hw_exit )){
		printf("dlopen did not find all required symbols\n");
		return EXIT_FAILURE;
	}


	if (strncmp(argv[argpos], "--broadband_type", sizeof("--broadband_type")) == 0){
		argpos ++;
		if ( argc < 4 ){

			print_usage();
			return EXIT_FAILURE;
		}

		if (strncmp(argv[argpos], "DSL", sizeof("DSL")) == 0){

			broadband_type = BROADBAND_TYPE_DSL;
			argpos++;
		}

		else if (strncmp(argv[argpos], "ATA", sizeof("ATA")) == 0){

			broadband_type = BROADBAND_TYPE_ATA;
			argpos++;
		}

		else {

			print_usage();
			return EXIT_FAILURE;
		}
	}

	if ( argpos > (argc-1) ){
		print_usage();
		return EXIT_FAILURE;
	}

	if (strncmp(argv[argpos], "hw_start_tm_atm", sizeof("hw_start_tm_atm")) == 0){
		hw_start_selected_tm( TRANSFER_MODE_ATM, broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_start_tm_atm_no_dsl_init", sizeof("hw_start_tm_atm_no_dsl_init")) == 0){
		hw_start_selected_tm_no_dsl_init ( TRANSFER_MODE_ATM, broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_start_tm_atm_no_dsl_init", sizeof("hw_start_tm_atm_no_dsl_init")) == 0){
		hw_start_selected_tm_no_dsl_init ( TRANSFER_MODE_ATM, broadband_type );
	}
	else if (strncmp(argv[argpos], "hw_start_tm_ptm", sizeof("hw_start_tm_ptm")) == 0){
		hw_start_selected_tm( TRANSFER_MODE_PTM, broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_disable_phy", sizeof("hw_disable_phy")) == 0){
		hw_disable_phy( broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_setup_ata_eth_dev", sizeof("hw_setup_ata_eth_dev")) == 0){
		char *dev_name = NULL;
		argpos++;
		if ( argpos <= (argc-1) ){
			dev_name = argv[argpos];
		}
		if (broadband_type == BROADBAND_TYPE_ATA ){
			hw_setup_ata_eth_dev( dev_name );
		} else {
			print_usage();
			return EXIT_FAILURE;
		}
	}

	else if (strncmp(argv[argpos], "hw_setup_ata_kdev", sizeof("hw_setup_ata_kdev")) == 0){
		char *dev_name = NULL;
		argpos++;
		if ( argpos <= (argc-1) ){
			dev_name = argv[argpos];
		}

		if (broadband_type == BROADBAND_TYPE_ATA ){
			hw_setup_ata_kdev( dev_name );
		} else {
			print_usage();
			return EXIT_FAILURE;
		}
	}

	else if (strncmp(argv[argpos], "hw_exit", sizeof("hw_exit")) == 0){
		hw_exit( broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_link_established_ptm", sizeof("hw_link_established_ptm")) == 0){
		hw_link_established( TRANSFER_MODE_PTM, broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_link_established_atm", sizeof("hw_link_established_atm")) == 0){
		hw_link_established( TRANSFER_MODE_ATM, broadband_type );
	}

	else if (strncmp(argv[argpos], "hw_link_established_atm_no_dsl_init", sizeof("hw_link_established_atm_no_dsl_init")) == 0){
		hw_link_established( TRANSFER_MODE_ATM, broadband_type );
	}

	else {
		print_usage();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

