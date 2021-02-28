/*
 * MIT License
 *
 * Copyright (c) 2021 David Smith
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#ifndef strlcpy
#  define strlcpy strncpy
#endif

const char *startfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static int units, board, border, corner, margin, fontsize,
	whitetomove, blacktomove, /* combine these into sidetomove */
	coords, grayscale, reverse, indicator;
static char pieces[64];
static char color_light[9];
static char color_dark[9];
static char font[64];

void
cat (const char* filename)
{
		// TODO: simplify this with `fread`
    // TODO; make this not need a fixed buffer , bytes at a time
    size_t len;
    char line[4096];
    FILE *f;
    if ((f = fopen(filename, "r")) == NULL)
        err(EX_NOINPUT, "%s", filename);
    while (fgets(line, 4096, f) != NULL)
        printf("%s", line);
    fclose(f);
}

char
sidetomove (const char *fen)
{
    return *(index(fen, ' ') + 1);
}

void
put_svg_header(const char *fen)
{
    int N = units*8 + 2*margin;
	printf("<?xml version='1.0' encoding='utf-8'?>\n"
           "<svg viewBox='0 0 %d %d' style='background-color:#ffffff00' version='1.1' "
           "xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' "
           "xml:space='preserve' x='0px' y='0px' width='%d' height='%d'>\n", N, N, N, N);
	printf("<!-- Creator: SVGBoard, Copyright 2019 David S. Smith <david.smith@gmail.com> -->\n");
	printf("<!-- FEN: %s -->\n", fen);
}

void
put_squares ()
{
    int N = 8*units;

    printf("<!-- DARK SQUARES -->\n");
    if (grayscale) // cross hatched:
    {
		int d = units/12;
        printf("<pattern id='crosshatch' width='%d' height='%d' "
               "patternTransform='rotate(45 0 0)' patternUnits='userSpaceOnUse'>\n"
                 "  <line x1='0' y1='0' x2='0' y2='%d' style='stroke:#000; stroke-width:1' />\n "
               "</pattern>\n", d, d, d);
                 //"  <rect x='0' y='0' width='%d' height='%d' fill='#fff' />\n "
		printf("<pattern id='diagonalHatch' patternUnits='userSpaceOnUse' width='4' height='4'>\n "
  				"<path d='M-1,1 l2,-2 M0,4 l4,-4 M3,5 l2,-2'\n "
        		"style='stroke:black; stroke-width:1' />\n "
				"</pattern>\n");
        printf("<rect x='%d' y='%d' width='%d' height='%d' fill='url(#crosshatch)' />\n",
                margin, margin, N, N);
        strlcpy(color_light, "fff", 4);
    }
    else // shaded
        printf("<rect x='%d' y='%d' width='%d' height='%d' fill='#%s' />\n",
                margin, margin, N, N, color_dark);

    printf("<!-- LIGHT SQUARES -->\n");
    printf("<pattern id='checkerboard' x='%d' y='%d' width='%d' height='%d' patternUnits='userSpaceOnUse'>\n",
            margin, margin, 2*units, 2*units);
    printf("  <rect x='0' y='0' width='%d' height='%d' style='fill:#%s;' />\n",
            units, units, color_light);
    printf("  <rect x='%d' y='%d' width='%d' height='%d' style='fill:#%s;' />\n",
            units, units, units, units, color_light);
    printf("</pattern>\n");
    printf("<rect x='%d' y='%d' width='%d' height='%d' fill='url(#checkerboard)' />\n",
            margin, margin, N, N);
}


void
put_border ()
{
    int n, N;
    int r = corner;
	int t = 0; //units / 32; // border gap thickness
    printf("<!-- BORDER -->\n");
    n = margin - t;
    N = board + 2*t;
    printf("<rect x='%d' y='%d' width='%d' height='%d' fill='none' "
            "stroke-width='%d' stroke-location='outside' stroke='#fff' rx='%d' ry='%d' />\n",
            n, n, N, N, t, r, r);
    n -= border == 1 ? border : border/2;
    N += border == 1 ? 2*border : border;
    printf("<rect x='%d' y='%d' width='%d' height='%d' fill='none' "
            "stroke-width='%d' stroke-location='outside' stroke='#000' rx='%d' ry='%d' />\n",
            n, n, N, N, border, r, r);
}


void
put_piece (const char c, const int x, const int y)
{
    char path[80];
    printf("<svg x='%d' y='%d' width='%d' height='%d'>\n", x, y, units, units);
    if (isupper(c))
        sprintf(path, "pieces/%s/w%c.svg", pieces, c);
    else
        sprintf(path, "pieces/%s/b%c.svg", pieces, toupper(c));
    cat(path);
    printf("</svg>\n");
}


void
put_move_indicator ()
{
    int cx = units*8 + 3*margin/2;
    int cy = margin + units/2;
    int dy = units/5, dx = units/5;
    int stroke = 1;

    if ((whitetomove && !reverse) || (blacktomove && reverse)) {
        cy += 7*units;
        dy = -dy;
    }
    if (whitetomove)
        stroke = 2;
    char *fill = whitetomove ? "fff" : "000";
    printf("<path d='M%d %d l%d %d l%d %d q%d %d %d %d' stroke='black' stroke-width='%d' fill='#%s' />\n",
            cx-dx, cy-dy, dx, 2*dy, dx, -2*dy, -dx, dy, -2*dx, 0, stroke, fill);
}


void
put_coords ()
{
    const char *files = "abcdefgh";
    int rank_y0 = margin + units/2 + fontsize/2;
    int rank_x0 = margin - fontsize - border;
    int file_x0 = margin + units/2 - fontsize/4;
    int file_y0 = margin + units*8 + border + fontsize;

    printf("<svg>\n");
    printf("<style> .small { font: normal %dpx %s; } </style>\n", fontsize, font);

    for (int file = 0; file < 8; ++file)
        printf("<text x=\"%d\" y=\"%d\" fill=\"#000\" class=\"small\">%c</text>\n",
            file_x0+units*file, file_y0, files[reverse ? 7-file : file]);
    for (int rank = 0; rank < 8; ++rank)
        printf("<text x=\"%d\" y=\"%d\" fill=\"#000\" class=\"small\">%d</text>\n",
            rank_x0, rank*units+rank_y0, reverse ? rank+1 : 8-rank);

    printf("</svg>\n");
}


void
build_board (const char *fen)
{
    whitetomove = (sidetomove(fen) == 'w');
    blacktomove = 1 - whitetomove;
    board = 8*units;
	int step = reverse ? -units : units;

    put_svg_header(fen);
    put_squares();
    put_border();

    if (coords)
        put_coords();

    int i = 0, x = margin, y = margin;
	if (reverse) {
		x += 7*units;
		y += 7*units;
	}
	puts("<!-- PIECES -->");
    for ( ; i < strlen(fen); ++i)
    {
        if (isalpha(fen[i])) {
            put_piece(fen[i], x, y);
            x += step;
        } else if (isdigit(fen[i]))
            x += step*atoi(&fen[i]);
        else if (fen[i] == '/') {
            x = reverse ? 7*units + margin : margin;
            y += step;
        } else if (fen[i] == ' ')
            break;
        else
            err(EX_DATAERR, "Weird FEN received: %s\n", fen);
    }
    if (indicator)
        put_move_indicator();
    puts("</svg>");
}


void
set_defaults ()
{
    coords = 0;
    reverse = 0;
    grayscale = 0;
    indicator = 0;
    strlcpy(pieces, "merida", sizeof pieces);
    units = 64;
    border = 2;
	corner = 0; //units/16;
    margin = 48;
    fontsize = 20;
    strlcpy(font, "sans-serif", 64);
    strlcpy(color_light, "a48c62", 7);
    strlcpy(color_dark, "846c40", 7);
}


void
print_usage()
{
    fprintf(stderr, "Usage: svgboard [-cghir] [-p <pieces>] FEN\n");
    fprintf(stderr, "\t-b\t\t border thickness (default: %d)\n", border);
    fprintf(stderr, "\t-c\t\t turn on coordinates (default: %d)\n", coords);
    fprintf(stderr, "\t-d\t\t dark square color (default: %s)\n", color_dark);
    fprintf(stderr, "\t-f\t\t font size (default: %d)\n", fontsize);
    fprintf(stderr, "\t-F\t\t font name (default: %s)\n", font);
    fprintf(stderr, "\t-g\t\t grayscale, suitable for print\n");
    fprintf(stderr, "\t-i\t\t show side-to-move indicator\n");
    fprintf(stderr, "\t-l\t\t light square color (default: %s)\n", color_light);
    fprintf(stderr, "\t-m\t\t margin size (default: %d)\n", margin);
    fprintf(stderr, "\t-p <pieces>\t pieces to use (default: %s)\n", pieces);
    fprintf(stderr, "\t-r \t\t flip if black to move instead of indicator\n");
    fprintf(stderr, "\t-s \t\t square size (default: %d)\n", units);
}

int
main (int argc, char *argv[])
{
    int c;

    set_defaults();

    while ((c = getopt(argc, argv, "b:cd:f:F:gil:m:p:rs:h")) != -1)
    {
        switch (c) {
        case 'b':
            border = atoi(optarg);
            break;
        case 'c':
            coords = 1;
            break;
        case 'd':
            strlcpy(color_dark, optarg, 9);
            break;
        case 'F':
            strlcpy(font, optarg, 64);
            break;
        case 'f':
            fontsize = atoi(optarg);
            break;
        case 'g':
            grayscale = 1;
            break;
        case 'i':
            indicator = 1;
            break;
        case 'l':
            strlcpy(color_light, optarg, 9);
            break;
        case 'm':
            margin = atoi(optarg);
            break;
        case 'p':
            strlcpy(pieces, optarg, sizeof pieces);
            break;
        case 'r':
            reverse = 1;
            break;
        case 's':
            units = atoi(optarg);
            break;
        case 'h':
        default:
            print_usage();
            return 1;
        }
    }
    argc -= optind;
    argv += optind;
    build_board(argc > 0 ? *argv : startfen);

    return 0;
}
