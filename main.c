#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <linux/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <ctype.h>

#include "eeprom.h"

#define VERSION_SUPPORTED 0

void showusage(char *pgmname)
{
	printf("Usage: %s [-f frequency (MHz)] [-s] file\n", pgmname);
	abort();
}

void print_entry(struct fmcomms1_calib_data *data)
{
	printf("Calibration Frequency:\t%d MHz\n", data->cal_frequency_MHz);

	printf("DAC I Phase Adjust:\t\t%d\n", data->i_phase_adj);
	printf("DAC Q Phase Adjust:\t\t%d\n", data->q_phase_adj);
	printf("DAC I Offset:\t\t%u\n", data->i_dac_offset);
	printf("DAC Q Offset:\t\t%u\n", data->q_dac_offset);
	printf("DAC I Full Scale Adj:\t%u\n", data->i_dac_fs_adj);
	printf("DAC Q Full Scale Adj:\t%u\n", data->q_dac_fs_adj);
	printf("ADC I Offset:\t\t%d\n", data->i_adc_offset_adj);
	printf("ADC Q Offset:\t\t%d\n", data->q_adc_offset_adj);
	printf("ADC I Gain Adj:\t%u\n", data->i_adc_gain_adj);
	printf("ADC Q Gain Adj:\t%u\n", data->q_adc_gain_adj);
}

void store_entry_hw(struct fmcomms1_calib_data *data)
{
	FILE *gp = popen("iio_cmdsrv","w");
	if (gp) {
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibbias %d\n", data->i_dac_offset);
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibscale %d\n", data->i_dac_fs_adj);
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_phase %d\n", data->i_phase_adj);
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibbias %d\n", data->q_dac_offset);
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibscale %d\n", data->q_dac_fs_adj);
		fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_phase %d\n", data->q_phase_adj);
		fflush(gp);

		fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibbias %d\n", data->i_adc_offset_adj);
		fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibscale %d\n", data->i_adc_gain_adj);
		fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibbias %d\n", data->q_adc_offset_adj);
		fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibscale %d\n", data->q_adc_gain_adj);
		fflush(gp);
		pclose(gp);
	} else {
		fprintf (stderr, "popen iio_cmdsrv failed %d\n", errno);
	}
}

int main (int argc , char* argv[])
{
	FILE *fp;
	struct fmcomms1_calib_data *data, *data2;
	int c, set = 0, len, ind = 0;
	int f = 0, delta, gindex;
	unsigned int min_delta = 0xFFFFFFFF;

	while ((c = getopt (argc, argv, "sf:")) != -1)
		switch (c) {
		case 'f':
			f = atoi(optarg);
			break;
		case 's':
			set = 1;
			break;
		case '?':
			if (optopt == 'c')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			return 1;
		default:
			abort ();
		}

	if (argv[optind] == NULL)
		showusage(argv[0]);

	fp = fopen(argv[optind], "r");
	if (fp == NULL) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	data = data2 = malloc(FAB_SIZE_CAL_EEPROM);
	if (data == NULL) {
		perror("malloc");
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	memset(data, 0, FAB_SIZE_CAL_EEPROM);

	len = fread(data, 1, FAB_SIZE_CAL_EEPROM, fp);
	if (len != FAB_SIZE_CAL_EEPROM) {
		perror("fread");
		goto out;
	}

	do {
		if (data->adi_magic0 != ADI_MAGIC_0 || data->adi_magic1 != ADI_MAGIC_1) {
			fprintf (stderr, "invalid magic detected\n");
			goto out;
		}
		if (data->version != ADI_VERSION(VERSION_SUPPORTED)) {
			fprintf (stderr, "unsupported version detected %c\n", data->version);
			goto out;
		}


		if (f) {
			delta = abs(f - data->cal_frequency_MHz);
			if (delta < min_delta) {
				gindex = ind;
				min_delta = delta;
			}

		} else {
			printf("\n--- ENTRY %d ---\n", ind);
			print_entry(data);
		}
		ind++;
	} while (data++->next);

	if (f) {
		printf("\n--- Best match ENTRY %d ---\n", gindex);
		print_entry(&data2[gindex]);
		if (set)
			store_entry_hw(&data2[gindex]);

	}

	fclose(fp);
	exit(EXIT_SUCCESS);

out:
	fclose(fp);
	free(data);
	exit(EXIT_FAILURE);
}
