#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <pulse/simple.h>
#include <fftw3.h>
#define BUFFER_SIZE 2048

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define WIDTH 1920
#define HEIGHT 300
#define EDGE 40
#define FOREGROUND 0x4C566A
#define BACKGROUND 0xFFFFFF

//Holds dimensions of a box
struct Box {
    int x;
    int y;
    int width;
    int height;
} typedef Box;

Display *dpy;
Window win;
GC gc;
Region r;

//void initRegion() {
//    r = XCreateRegion();
//    XRectangle rectangle;
//    XUnionRectWithRegion();
//    XSetRegion(dpy, gc, r);
//
//}

void initWindow() {
    dpy = XOpenDisplay(NULL);
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, WIDTH, HEIGHT, 0, 0x000000, 0xFFFFFF);
    //win = DefaultRootWindow(dpy);
    gc = XCreateGC(dpy, win, 0, NULL);
    XMapWindow(dpy, win);
    XFlush(dpy);
    usleep(20000);
}

Box * initBoxes (int g, int * n, int * m) {

    int framed = WIDTH - 2 * EDGE;

    *m = 1;

    //if boxes don't fit increase the width to integer value
    int w = (int)ceil((double)framed / *n);

    for(; (w - g) < 4; (*m)++) {
        *n = *n / *m;
        w = (int)ceil((double)framed / *n);
    }
    //remove excess boxes that go over edge
    *n -=  ((w * *n) - framed) * ((w * *n) > framed) / w;

    Box * boxArray = (Box *)malloc(sizeof(Box)* *n);

    for(int i = 0; i < *n; i++) {
        boxArray[i].width = w - g;
        boxArray[i].height = 0;
        boxArray[i].x = EDGE + i * w;
        boxArray[i].y = EDGE;
    }

    return boxArray;
}

void clear(Box box) {
    XSetForeground(dpy, gc, BACKGROUND);
    XFillRectangle(dpy, win, gc, box.x, box.y, box.width, box.height);
}
void paint(Box box) {
    XSetForeground(dpy, gc, FOREGROUND);
    XFillRectangle(dpy, win, gc, box.x, box.y, box.width, box.height);
}

int main(int argc, char **argv) {
    fftw_complex *in, *out;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * BUFFER_SIZE);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * BUFFER_SIZE);
    fftw_plan plan = fftw_plan_dft_1d(BUFFER_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    pa_simple *s;
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = 44100;

    pa_buffer_attr attr;
    attr.fragsize = pa_usec_to_bytes(12000, &ss);

    char * source = "alsa_output.pci-0000_00_1f.3.analog-stereo.monitor";

    if (argc > 1) {
        source = argv[1];
    }

    s = pa_simple_new(NULL,
                      "Visualizer",
                      PA_STREAM_RECORD,
                      source,
                      "Output",
                      &ss,
                      NULL,
                      &attr,
                      NULL);

    int16_t buf[BUFFER_SIZE];

    int scale = 2;
    int n = BUFFER_SIZE / 12; // number of columns in visualization

    Box * boxes = initBoxes(2, &n, &scale); // initializes dimensions of n boxes

    double * heights = (double *)calloc(n, sizeof(double)); //measured heights of boxes

    int yLimit = HEIGHT - 2 * EDGE;

    initWindow();

    while(1) {
        pa_simple_read(s, buf, BUFFER_SIZE * 2, NULL);

        for (int i = 0; i < BUFFER_SIZE; i++) {
            in[i][0] = (double)buf[i];
            in[i][1] = 0;
        }

        fftw_execute(plan);

        for (int i = 0; i < n; i++) {
            heights[i] = 0;
            for(int j = 0; j < scale; j++) {
                //printf("%d\n", (scale * i) + j);
                heights[i] += fabs(out[(scale * i) + j][0]);
                heights[i] = heights[i] * sqrt(scale) / scale;
            }
            clear(boxes[i]);
            //incremental instead of entire change used to smoothen animation
            boxes[i].height = ((int)((heights[i] / 3000000) * yLimit) - boxes[i].height / 4) + boxes[i].height;
            //box height will cap out at the window limit
            boxes[i].height > yLimit ? boxes[i].height = yLimit: 0;
            //adjusting positional value of box
            boxes[i].y = HEIGHT - EDGE - boxes[i].height;

            paint(boxes[i]);
        }
        XFlush(dpy);
    }
    pa_simple_free(s);

    fftw_free(in);
    fftw_free(out);
    fftw_destroy_plan(plan);

    return 0;
}
