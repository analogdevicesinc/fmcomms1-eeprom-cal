/*
 * fmcomms1-eeprom-cal
 *
 * Copyright 2012-2014 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */
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
	printf("Usage: %s [-f frequency (MHz)] [-s -r -t] file\n", pgmname);
	abort();
}

void print_entry(struct fmcomms1_calib_data *data)
{
	printf("Calibration Frequency: %d MHz\n", data->cal_frequency_MHz);

	printf("DAC I Phase Adjust:    %d\n", data->i_phase_adj);
	printf("DAC Q Phase Adjust:    %d\n", data->q_phase_adj);
	printf("DAC I Offset:          %u\n", data->i_dac_offset);
	printf("DAC Q Offset:          %u\n", data->q_dac_offset);
	printf("DAC I Full Scale Adj:  %u\n", data->i_dac_fs_adj);
	printf("DAC Q Full Scale Adj:  %u\n", data->q_dac_fs_adj);
	printf("ADC I Offset:          %d\n", data->i_adc_offset_adj);
	printf("ADC Q Offset:          %d\n", data->q_adc_offset_adj);
	printf("ADC I Gain Adj:        %f\n", fract1_15_to_float(data->i_adc_gain_adj));
	printf("ADC Q Gain Adj:        %f\n", fract1_15_to_float(data->q_adc_gain_adj));
}

void print_entry_v1(struct fmcomms1_calib_data_v1 *data)
{
	printf("Calibration Frequency: %d MHz\n", data->cal_frequency_MHz);

	printf("DAC I Phase Adjust:    %d\n", data->i_phase_adj);
	printf("DAC Q Phase Adjust:    %d\n", data->q_phase_adj);
	printf("DAC I Offset:          %d\n", data->i_dac_offset);
	printf("DAC Q Offset:          %d\n", data->q_dac_offset);
	printf("DAC I Full Scale Adj:  %u\n", data->i_dac_fs_adj);
	printf("DAC Q Full Scale Adj:  %u\n", data->q_dac_fs_adj);
	printf("ADC I Offset:          %d\n", data->i_adc_offset_adj);
	printf("ADC Q Offset:          %d\n", data->q_adc_offset_adj);
	printf("ADC I Gain Adj:        %f\n", fract1_1_14_to_float(data->i_adc_gain_adj));
	printf("ADC Q Gain Adj:        %f\n", fract1_1_14_to_float(data->q_adc_gain_adj));
	printf("ADC I Phase Adj:       %f\n", fract1_1_14_to_float(data->i_adc_phase_adj));

}

void store_entry_hw(struct fmcomms1_calib_data *data, unsigned tx, unsigned rx)
{

	if (!(tx || rx))
		return;

	FILE *gp = popen("iio_cmdsrv","w");
	if (gp) {
		if (tx) {
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibbias %d\n", data->i_dac_offset);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibscale %d\n", data->i_dac_fs_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_phase %d\n", data->i_phase_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibbias %d\n", data->q_dac_offset);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibscale %d\n", data->q_dac_fs_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_phase %d\n", data->q_phase_adj);
			fflush(gp);
		}
		if (rx) {
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibbias %d\n", data->i_adc_offset_adj);
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibscale %f\n", fract1_15_to_float(data->i_adc_gain_adj));
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibphase %f\n", 0.0);
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibbias %d\n", data->q_adc_offset_adj);
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibscale %f\n", fract1_15_to_float(data->q_adc_gain_adj));
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibphase %f\n", 0.0);
			fflush(gp);
		}
		pclose(gp);
	} else {
		fprintf (stderr, "popen iio_cmdsrv failed %d\n", errno);
	}
}

void store_entry_hw_v1(struct fmcomms1_calib_data_v1 *data, unsigned tx,
		       unsigned rx, unsigned short temp_calibbias)
{

	if (!(tx || rx))
		return;

	FILE *gp = popen("iio_cmdsrv","w");
	if (gp) {

		fprintf(gp, "write cf-ad9122-core-lpc in_temp0_calibbias %d\n", temp_calibbias);

		if (tx) {
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibbias %d\n", data->i_dac_offset);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_calibscale %d\n", data->i_dac_fs_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage0_phase %d\n", data->i_phase_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibbias %d\n", data->q_dac_offset);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_calibscale %d\n", data->q_dac_fs_adj);
			fprintf(gp, "write cf-ad9122-core-lpc out_voltage1_phase %d\n", data->q_phase_adj);
			fflush(gp);
		}
		if (rx) {
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibbias %d\n", data->i_adc_offset_adj);
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibscale %f\n", fract1_1_14_to_float(data->i_adc_gain_adj));
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage0_calibphase %f\n", fract1_1_14_to_float(data->i_adc_phase_adj));
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibbias %d\n", data->q_adc_offset_adj);
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibscale %f\n", fract1_1_14_to_float(data->q_adc_gain_adj));
			fprintf(gp, "write cf-ad9643-core-lpc in_voltage1_calibphase %f\n", 0.0);
			fflush(gp);
		}
		pclose(gp);
	} else {
		fprintf (stderr, "popen iio_cmdsrv failed %d\n", errno);
	}
}

int main (int argc , char* argv[])
{
	FILE *fp;
	struct fmcomms1_calib_header_v1 *header;
	char *buf;
	int c, set_tx = 0, set_rx = 0, len, ind = 0;
	int f = 0, delta, gindex;
	int min_delta = 2147483647;

	while ((c = getopt (argc, argv, "strf:")) != -1)
		switch (c) {
		case 'f':
			f = atoi(optarg);
			break;
		case 's':
			set_tx = 1;
			set_rx = 1;
			break;
		case 't':
			set_tx = 1;
			break;
		case 'r':
			set_rx = 1;
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

	buf = malloc(FAB_SIZE_CAL_EEPROM);
	if (buf == NULL) {
		perror("malloc");
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	memset(buf, 0, FAB_SIZE_CAL_EEPROM);

	len = fread(buf, 1, FAB_SIZE_CAL_EEPROM, fp);
	if (len != FAB_SIZE_CAL_EEPROM) {
		perror("fread");
		goto out;
	}

	header = (struct fmcomms1_calib_header_v1 *) buf;

	if (header->adi_magic0 != ADI_MAGIC_0 || header->adi_magic1 != ADI_MAGIC_1) {
		fprintf (stderr, "invalid magic detected\n");
		goto out;
	}

	printf("\nFound Version %c \n", header->version);

	if (header->version == ADI_VERSION(0)) {
		struct fmcomms1_calib_data *data, *data2;
		data = data2 = (struct fmcomms1_calib_data *)buf;

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
			store_entry_hw(&data2[gindex], set_tx, set_rx);
		}

	} else if (header->version == ADI_VERSION(1)) {
		struct fmcomms1_calib_data_v1 *data =
			(struct fmcomms1_calib_data_v1 *) (buf + sizeof(*header));

		for (ind = 0; ind < header->num_entries; ind++) {

			if (f) {
				delta = abs(f - data[ind].cal_frequency_MHz);
				if (delta < min_delta) {
					gindex = ind;
					min_delta = delta;
				}

			} else {
				printf("\n--- ENTRY %d ---\n", ind);
				print_entry_v1(&data[ind]);
			}
		};

		if (f) {
			printf("\n--- Best match ENTRY %d ---\n", gindex);
			print_entry_v1(&data[gindex]);
			store_entry_hw_v1(&data[gindex], set_tx, set_rx,
					  header->temp_calibbias);
		}
	} else {
		fprintf (stderr, "unsupported version detected %c\n", header->version);
		goto out;
	}

	fclose(fp);
	exit(EXIT_SUCCESS);

out:
	fclose(fp);
	free(buf);
	exit(EXIT_FAILURE);
}
