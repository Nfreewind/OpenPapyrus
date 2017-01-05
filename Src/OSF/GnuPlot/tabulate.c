/* GNUPLOT - tabulate.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/

/*
 * EAM - April 2007
 * Collect the routines for tablular output into a separate file, and
 * extend support to newer plot styles (labels, vectors, ...).
 * These routines used to masquerade as a terminal type "set term table",
 * but since version 4.2 they are controlled by "set table <foo>" independent
 * of terminal settings.
 *
 * BM - April 2014
 * Support output to an in-memory named datablock.
 */
#include <gnuplot.h>
#pragma hdrstop

// File or datablock for output during 'set table' mode 
FILE * table_outfile = NULL;
UdvtEntry * table_var = NULL;
bool table_mode = false;
static FILE * outfile;
static char * expand_newline(const char * in);

#define OUTPUT_NUMBER(x, y) { output_number(x, y, buffer); len = strappend(&line, &size, len, buffer); }
#define BUFFERSIZE 128

static void output_number(double coord, int axis, char * buffer) 
{
	if(fisnan(coord)) {
		sprintf(buffer, " NaN");

		/* treat timedata and "%s" output format as a special case:
		 * return a number.
		 * "%s" in combination with any other character is treated
		 * like a normal time format specifier and handled in time.c
		 */
	}
	else if(GpGg[axis].tictype == DT_TIMEDATE && strcmp(GpGg[axis].formatstring, "%s") == 0) {
		gprintf(buffer, BUFFERSIZE, "%.0f", 1.0, coord);
	}
	else if(GpGg[axis].tictype == DT_TIMEDATE) {
		buffer[0] = '"';
		if(!strcmp(GpGg[axis].formatstring, DEF_FORMAT))
			gstrftime(buffer+1, BUFFERSIZE-1, GpGg.P_TimeFormat, coord);
		else
			gstrftime(buffer+1, BUFFERSIZE-1, GpGg[axis].formatstring, coord);
		while(strchr(buffer, '\n')) {
			*(strchr(buffer, '\n')) = ' ';
		}
		strcat(buffer, "\"");
	}
	else if(GpGg[axis].Flags & GpAxis::fLog) {
		double x = pow(GpGg[axis].base, coord);
		gprintf(buffer, BUFFERSIZE, GpGg[axis].formatstring, 1.0, x);
	}
	else
		gprintf(buffer, BUFFERSIZE, GpGg[axis].formatstring, 1.0, coord);
	strcat(buffer, " ");
}

static void print_line(const char * str)
{
	if(table_var == NULL) {
		fputs(str, outfile);
		fputc('\n', outfile);
	}
	else {
		append_to_datablock(&table_var->udv_value, _strdup(str));
	}
}

void print_table(CurvePoints * current_plot, int plot_num)
{
	int i, curve;
	char * buffer = (char*)malloc(BUFFERSIZE);
	size_t size = 2*BUFFERSIZE;
	char * line = (char*)malloc(size);
	size_t len = 0;
	outfile = (table_outfile) ? table_outfile : gpoutfile;
	for(curve = 0; curve < plot_num; curve++, current_plot = current_plot->P_Next) {
		GpCoordinate * point = NULL;
		if(current_plot->plot_style != TABLESTYLE) { // "with table" already wrote the output 
			// two blank lines between tabulated plots by prepending an empty line here 
			print_line("");
			snprintf(line, size, "# Curve %d of %d, %d points", curve, plot_num, current_plot->p_count);
			print_line(line);
			if((current_plot->title) && (*current_plot->title)) {
				char * title = expand_newline(current_plot->title);
				snprintf(line, size, "# Curve title: \"%s\"", title);
				print_line(line);
				free(title);
			}
			len = snprintf(line, size, "# x y");
			switch(current_plot->plot_style) {
				case BOXES:
				case XERRORBARS:
					len = strappend(&line, &size, len, " xlow xhigh");
					break;
				case BOXERROR:
				case YERRORBARS:
					len = strappend(&line, &size, len, " ylow yhigh");
					break;
				case BOXXYERROR:
				case XYERRORBARS:
					len = strappend(&line, &size, len, " xlow xhigh ylow yhigh");
					break;
				case FILLEDCURVES:
					len = strappend(&line, &size, len, "1 y2");
					break;
				case FINANCEBARS:
					len = strappend(&line, &size, len, " open ylow yhigh yclose");
					break;
				case CANDLESTICKS:
					len = strappend(&line, &size, len, " open ylow yhigh yclose width");
					break;
				case LABELPOINTS:
					len = strappend(&line, &size, len, " label");
					break;
				case VECTOR:
					len = strappend(&line, &size, len, " delta_x delta_y");
					break;
				case LINES:
				case POINTSTYLE:
				case LINESPOINTS:
				case DOTS:
				case IMPULSES:
				case STEPS:
				case FSTEPS:
				case HISTEPS:
					break;
				case IMAGE:
					len = strappend(&line, &size, len, "  pixel");
					break;
				case RGBIMAGE:
				case RGBA_IMAGE:
					len = strappend(&line, &size, len, "  red green blue alpha");
					break;
				default:
					if(GpGg.IsInteractive)
						fprintf(stderr, "Tabular output of %s plot style not fully implemented\n",
							current_plot->plot_style == HISTOGRAMS ? "histograms" : "this");
					break;
			}
			if(current_plot->varcolor)
				len = strappend(&line, &size, len, "  color");
			strappend(&line, &size, len, " type");
			print_line(line);
			if(current_plot->plot_style == LABELPOINTS) {
				for(GpTextLabel * this_label = current_plot->labels->next; this_label != NULL; this_label = this_label->next) {
					char * label = expand_newline(this_label->text);
					line[0] = NUL;
					len = 0;
					OUTPUT_NUMBER(this_label->place.x, current_plot->x_axis);
					OUTPUT_NUMBER(this_label->place.y, current_plot->y_axis);
					len = strappend(&line, &size, len, "\"");
					len = strappend(&line, &size, len, label);
					len = strappend(&line, &size, len, "\"");
					print_line(line);
					free(label);
				}
			}
			else {
				int plotstyle = current_plot->plot_style;
				if(plotstyle == HISTOGRAMS && current_plot->histogram->type == HT_ERRORBARS)
					plotstyle = YERRORBARS;
				for(i = 0, point = current_plot->points; i < current_plot->p_count; i++, point++) {
					// Reproduce blank lines read from original input file, if any 
					if(memcmp(point, &GpDf.blank_data_line, sizeof(GpCoordinate)) == 0) {
						print_line("");
					}
					else {
						// FIXME HBB 20020405: had better use the real x/x2 axes of this plot 
						line[0] = NUL;
						len = 0;
						OUTPUT_NUMBER(point->x, current_plot->x_axis);
						OUTPUT_NUMBER(point->y, current_plot->y_axis);
						switch(plotstyle) {
							case BOXES:
							case XERRORBARS:
								OUTPUT_NUMBER(point->xlow,  current_plot->x_axis);
								OUTPUT_NUMBER(point->xhigh, current_plot->x_axis);
								/* Hmmm... shouldn't this write out width field of box plots, too, if stored? */
								break;
							case BOXXYERROR:
							case XYERRORBARS:
								OUTPUT_NUMBER(point->xlow, current_plot->x_axis);
								OUTPUT_NUMBER(point->xhigh, current_plot->x_axis);
							/* FALLTHROUGH */
							case BOXERROR:
							case YERRORBARS:
								OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
								OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
								break;
							case IMAGE:
								snprintf(buffer, BUFFERSIZE, "%g ", point->z);
								len = strappend(&line, &size, len, buffer);
								break;
							case RGBIMAGE:
							case RGBA_IMAGE:
								snprintf(buffer, BUFFERSIZE, "%4d %4d %4d %4d ",
								(int)point->CRD_R, (int)point->CRD_G,
								(int)point->CRD_B, (int)point->CRD_A);
								len = strappend(&line, &size, len, buffer);
								break;
							case FILLEDCURVES:
								OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
								break;
							case FINANCEBARS:
								OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
								OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
								OUTPUT_NUMBER(point->z, current_plot->y_axis);
								break;
							case CANDLESTICKS:
								OUTPUT_NUMBER(point->ylow, current_plot->y_axis);
								OUTPUT_NUMBER(point->yhigh, current_plot->y_axis);
								OUTPUT_NUMBER(point->z, current_plot->y_axis);
								OUTPUT_NUMBER(2. * (point->x - point->xlow), current_plot->x_axis);
								break;
							case VECTOR:
								OUTPUT_NUMBER((point->xhigh - point->x), current_plot->x_axis);
								OUTPUT_NUMBER((point->yhigh - point->y), current_plot->y_axis);
								break;
							case LINES:
							case POINTSTYLE:
							case LINESPOINTS:
							case DOTS:
							case IMPULSES:
							case STEPS:
							case FSTEPS:
							case HISTEPS:
								break;
							default:
								/* ? */
								break;
						}
						if(current_plot->varcolor) {
							double colorval = current_plot->varcolor[i];
							if((current_plot->lp_properties.pm3d_color.value < 0.0) && (current_plot->lp_properties.pm3d_color.type == TC_RGB)) {
								snprintf(buffer, BUFFERSIZE, "0x%06x", (uint)(colorval));
								len = strappend(&line, &size, len, buffer);
							}
							else if(current_plot->lp_properties.pm3d_color.type == TC_Z) {
								OUTPUT_NUMBER(colorval, COLOR_AXIS);
							}
							else if(current_plot->lp_properties.l_type == LT_COLORFROMCOLUMN) {
								OUTPUT_NUMBER(colorval, COLOR_AXIS);
							}
						}
						snprintf(buffer, BUFFERSIZE, " %c", (current_plot->points[i].type == INRANGE) ? 'i' : ((current_plot->points[i].type == OUTRANGE) ? 'o' : 'u'));
						strappend(&line, &size, len, buffer);
						print_line(line);
					}
				}
			}
			print_line("");
		}
	}
	if(outfile)
		fflush(outfile);
	free(buffer);
	free(line);
}

void print_3dtable(int pcount)
{
	SurfacePoints * this_plot;
	int i, surface;
	GpCoordinate * point;
	GpCoordinate * tail;
	char * buffer = (char*)malloc(BUFFERSIZE);
	size_t size = 2*BUFFERSIZE;
	char * line = (char*)malloc(size);
	size_t len = 0;
	outfile = (table_outfile) ? table_outfile : gpoutfile;
	for(surface = 0, this_plot = GpGg.P_First3DPlot; surface < pcount; this_plot = this_plot->next_sp, surface++) {
		print_line("");
		snprintf(line, size, "# Surface %d of %d surfaces", surface, pcount);
		print_line(line);
		if((this_plot->title) && (*this_plot->title)) {
			char * title = expand_newline(this_plot->title);
			print_line("");
			snprintf(line, size, "# Curve title: \"%s\"", title);
			print_line(line);
			free(title);
		}
		switch(this_plot->plot_style) {
			case LABELPOINTS:
		    {
			    GpTextLabel * this_label;
			    for(this_label = this_plot->labels->next; this_label != NULL; this_label = this_label->next) {
				    char * label = expand_newline(this_label->text);
				    line[0] = NUL;
				    len = 0;
				    OUTPUT_NUMBER(this_label->place.x, FIRST_X_AXIS);
				    OUTPUT_NUMBER(this_label->place.y, FIRST_Y_AXIS);
				    OUTPUT_NUMBER(this_label->place.z, FIRST_Z_AXIS);
				    len = strappend(&line, &size, len, "\"");
				    len = strappend(&line, &size, len, label);
				    len = strappend(&line, &size, len, "\"");
				    print_line(line);
				    free(label);
			    }
		    }
			    continue;
			case LINES:
			case POINTSTYLE:
			case IMPULSES:
			case DOTS:
			case VECTOR:
			case IMAGE:
			    break;
			default:
			    fprintf(stderr, "Tabular output of this 3D plot style not implemented\n");
			    continue;
		}
		if(GpGg.draw_surface) {
			iso_curve * icrvs;
			int curve;
			// only the curves in one direction 
			for(curve = 0, icrvs = this_plot->iso_crvs; icrvs && curve < this_plot->num_iso_read; icrvs = icrvs->next, curve++) {
				print_line("");
				snprintf(line, size, "# IsoCurve %d, %d points", curve, icrvs->p_count);
				print_line(line);
				len = sprintf(line, "# x y z");
				if(this_plot->plot_style == VECTOR) {
					tail = icrvs->next->points;
					len = strappend(&line, &size, len, " delta_x delta_y delta_z");
				}
				else {
					tail = NULL; /* Just to shut up a compiler warning */
				}
				strappend(&line, &size, len, " type");
				print_line(line);
				for(i = 0, point = icrvs->points; i < icrvs->p_count; i++, point++) {
					line[0] = NUL;
					len = 0;
					OUTPUT_NUMBER(point->x, FIRST_X_AXIS);
					OUTPUT_NUMBER(point->y, FIRST_Y_AXIS);
					OUTPUT_NUMBER(point->z, FIRST_Z_AXIS);
					if(this_plot->plot_style == VECTOR) {
						OUTPUT_NUMBER((tail->x - point->x), FIRST_X_AXIS);
						OUTPUT_NUMBER((tail->y - point->y), FIRST_Y_AXIS);
						OUTPUT_NUMBER((tail->z - point->z), FIRST_Z_AXIS);
						tail++;
					}
					else if(this_plot->plot_style == IMAGE) {
						snprintf(buffer, BUFFERSIZE, "%g ", point->CRD_COLOR);
						len = strappend(&line, &size, len, buffer);
					}
					snprintf(buffer, BUFFERSIZE, "%c", (point->type == INRANGE) ? 'i' : ((point->type == OUTRANGE) ? 'o' : 'u'));
					strappend(&line, &size, len, buffer);
					print_line(line);
				} /* for (point) */
			} /* for (icrvs) */
			print_line("");
		}
		if(GpGg.draw_contour) {
			int number = 0;
			gnuplot_contours * c = this_plot->contours;
			while(c) {
				int count = c->num_pts;
				GpCoordinate * point = c->coords;
				if(c->isNewLevel)
					/* don't display count - contour split across chunks */
					/* put # in case user wants to use it for a plot */
					/* double blank line to allow plot ... index ... */
					print_line("");
				snprintf(line, size, "# Contour %d, label: %s", number++, c->label);
				print_line(line);
				for(; --count >= 0; ++point) {
					line[0] = NUL;
					len = 0;
					OUTPUT_NUMBER(point->x, FIRST_X_AXIS);
					OUTPUT_NUMBER(point->y, FIRST_Y_AXIS);
					OUTPUT_NUMBER(point->z, FIRST_Z_AXIS);
					print_line(line);
				}

				/* blank line between segments of same contour */
				print_line("");
				c = c->next;
			} /* while (contour) */
		} /* if(draw_contour) */
	} /* for(surface) */
	if(outfile)
		fflush(outfile);
	free(buffer);
	free(line);
}

static char * expand_newline(const char * in)
{
	char * tmpstr = (char*)malloc(2 * strlen(in));
	const char * s = in;
	char * t = tmpstr;
	do {
		if(*s == '\n') {
			*t++ = '\\';
			*t++ = 'n';
		}
		else
			*t++ = *s;
	} while(*s++);
	return tmpstr;
}
