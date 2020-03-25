
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <portaudio.h>

inline int min(int a, int b) {
    return a < b ? a : b;
}

// All of my samples are 44.1kHz 2Channel 16bit audio files
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BIT_SIZE 16

#define SAMPLE_MAX SHRT_MAX
typedef short SampleType;

typedef struct {
    int sample_count;
    SampleType (*data)[CHANNELS];
} SoundData;

// Notes are indexed from 0 starting at C0 ang incrementing by one semitone
typedef int Note;

SoundData loadWavFileForNote(Note note) {
    SoundData sound_data = { 0, NULL };
    char name_buffer[1024];
    snprintf(name_buffer, 1024, "samples/%i.wav", note);
    FILE* file = fopen(name_buffer, "r");
    if(file != NULL) {
        uint8_t size_buffer[4];
        fseek(file, 40, SEEK_SET);
        fread(size_buffer, 1, 4, file);
        int size = 0;
        for(int i = 0; i < 4; i++) {
            size |= (int)(size_buffer[i]) << (i*8);
        }
        sound_data.sample_count = size / (BIT_SIZE / 8)  / CHANNELS;
        sound_data.data = (SampleType(*)[CHANNELS])malloc(sizeof(SampleType) * CHANNELS * sound_data.sample_count);
        for (int s = 0; s < sound_data.sample_count; s++) {
            for(int c = 0; c < CHANNELS; c++) {
                uint8_t value_buffer[(BIT_SIZE / 8)];
                fread(value_buffer, 1, (BIT_SIZE / 8), file);
                SampleType value = 0;
                for(int i = 0; i < (BIT_SIZE / 8); i++) {
                    value |= (int)(value_buffer[i]) << (i*8);
                }
                sound_data.data[s][c] = value;
            }
        }
        fclose(file);
    }
    return sound_data;
}

typedef struct {
    bool pressed;
    bool faiding;
    int sample_position;
    int fading_count;
} PlayingState;

typedef struct {
    SoundData sound_data;
    PlayingState playing_state;
} PianoString;

#define BASE_NOTE 9
#define NUMBER_OF_NOTES 84
PianoString piano[NUMBER_OF_NOTES];

void initPiano() {
    for(int i = 0; i < NUMBER_OF_NOTES; i++) {
        piano[i].sound_data = loadWavFileForNote(BASE_NOTE + i);
    }
}

void deinitPiano() {
    for(int i = 0; i < NUMBER_OF_NOTES; i++) {
        free(piano[i].sound_data.data);
    }
}

void pressPianoKey(Note note) {
    if(note >= BASE_NOTE && note < NUMBER_OF_NOTES + BASE_NOTE) {
        piano[note - BASE_NOTE].playing_state.sample_position = 0;
        piano[note - BASE_NOTE].playing_state.pressed = true;
    }
}

void depressPianoKey(Note note) {
    if(note >= BASE_NOTE && note < NUMBER_OF_NOTES + BASE_NOTE && piano[note - BASE_NOTE].playing_state.pressed) {
        piano[note - BASE_NOTE].playing_state.fading_count = 0;
        piano[note - BASE_NOTE].playing_state.faiding = true;
        piano[note - BASE_NOTE].playing_state.pressed = false;
    }
}

const char* note_to_string[] = {
    "C%i", "C#%i", "D%i", "D#%i", "E%i", "F%i", "F#%i", "G%i", "G#%i", "A%i", "A#%i", "B%i"
};
void noteToString(Note note, char* str) {
    int octave = note / 12;
    int off = note % 12;
    sprintf(str, note_to_string[off], octave);
}

void printPianoState() {
    fprintf(stdout, "\e[K");
    char str_buffer[10];
    for(int ps = 0; ps < NUMBER_OF_NOTES; ps++) {
        if(piano[ps].playing_state.pressed) {
            noteToString(ps + BASE_NOTE, str_buffer);
            fprintf(stdout, "%s ", str_buffer);
        }
    }
    fprintf(stdout, "\n\e[A");
}

void printPianoStateSheet() {
    fprintf(stdout, "\e[J");
    char str_buffer[10];
    fprintf(stdout, piano[79 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[80 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[77 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[78 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[76 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[74 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[75 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[72 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[73 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[71 - BASE_NOTE].playing_state.pressed ? "   O   \n" : "       \n");
    fprintf(stdout, piano[69 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[70 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[67 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[68 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[65 - BASE_NOTE].playing_state.pressed ? "---O---\n" : piano[66 - BASE_NOTE].playing_state.pressed ? "#--O---\n" : "-------\n");
    fprintf(stdout, piano[64 - BASE_NOTE].playing_state.pressed ? "   O   \n" : "       \n");
    fprintf(stdout, piano[62 - BASE_NOTE].playing_state.pressed ? "---O---\n" : piano[63 - BASE_NOTE].playing_state.pressed ? "#--O---\n" : "-------\n");
    fprintf(stdout, piano[60 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[61 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[59 - BASE_NOTE].playing_state.pressed ? "---O---\n" : "-------\n");
    fprintf(stdout, piano[57 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[58 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[55 - BASE_NOTE].playing_state.pressed ? "---O---\n" : piano[56 - BASE_NOTE].playing_state.pressed ? "#--O---\n" : "-------\n");
    fprintf(stdout, piano[53 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[54 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[52 - BASE_NOTE].playing_state.pressed ? "---O---\n" : "-------\n");
    fprintf(stdout, piano[50 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[51 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[48 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[49 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[47 - BASE_NOTE].playing_state.pressed ? "   O   \n" : "       \n");
    fprintf(stdout, piano[45 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[46 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[43 - BASE_NOTE].playing_state.pressed ? "   O   \n" : piano[44 - BASE_NOTE].playing_state.pressed ? "#  O   \n" : "       \n");
    fprintf(stdout, piano[41 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[42 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, piano[40 - BASE_NOTE].playing_state.pressed ? "   O   \n" : "       \n");
    fprintf(stdout, piano[38 - BASE_NOTE].playing_state.pressed ? "  -O-  \n" : piano[39 - BASE_NOTE].playing_state.pressed ? "# -O-  \n" : "  ---  \n");
    fprintf(stdout, "\n\e[26A");
}

PaStream* audio_stream;

int audioCallback(const void* input, void* output, uint64_t frame_count, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flage, void* user_data) {
    float (*out)[2] = (float(*)[2])output;
    for(int s = 0; s < frame_count; s++) {
        out[s][0] = 0;
        out[s][1] = 0;
    }
    for(int ps = 0; ps < NUMBER_OF_NOTES; ps++) {
        if(piano[ps].playing_state.pressed) {
            int current_position = piano[ps].playing_state.sample_position;
            int frames = min(frame_count, piano[ps].sound_data.sample_count - current_position);
            for(int s = 0; s < frames; s++) {
                out[s][0] += ((float)(piano[ps].sound_data.data[current_position + s][0]) / (float)SAMPLE_MAX / 10.);
                out[s][1] += ((float)(piano[ps].sound_data.data[current_position + s][1]) / (float)SAMPLE_MAX / 10.);
            }
            if(frames != frame_count) {
                piano[ps].playing_state.pressed = false;
            } else {
                piano[ps].playing_state.sample_position += frame_count;
            }
        } else if(piano[ps].playing_state.faiding) {
            int current_position = piano[ps].playing_state.sample_position;
            int fading_count = piano[ps].playing_state.fading_count;
            int frames = min(frame_count, piano[ps].sound_data.sample_count - current_position);
            for(int s = 0; s < frames; s++) {
                fading_count++;
                out[s][0] += ((float)(piano[ps].sound_data.data[current_position + s][0]) / (float)SAMPLE_MAX / 10. / (1 + (float)fading_count / 3000.));
                out[s][1] += ((float)(piano[ps].sound_data.data[current_position + s][1]) / (float)SAMPLE_MAX / 10. / (1 + (float)fading_count / 3000.));
            }
            if(frames != frame_count || fading_count > 44100) {
                piano[ps].playing_state.faiding = false;
            } else {
                piano[ps].playing_state.fading_count = fading_count;
                piano[ps].playing_state.sample_position += frame_count;
            }
        }
    }
    return 0;
}

void initPianoAudio() {
    freopen("/dev/null","w",stderr);
    Pa_Initialize();
    freopen("/dev/tty","w",stderr);
    Pa_OpenDefaultStream(&audio_stream, 0, CHANNELS, paFloat32, SAMPLE_RATE, paFramesPerBufferUnspecified, audioCallback, NULL);
    Pa_StartStream(audio_stream);
}

void deinitPianoAudio() {
    Pa_StopStream(audio_stream);
    Pa_Terminate();
}

int keycode_to_note[256];
const int keys[] = {
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 95, 96,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 51,
    94, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
};

void initKeys(Display* display) {
    Window root_window = DefaultRootWindow(display);

    for (int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        keycode_to_note[keys[i]] = i;
    }
    keycode_to_note[XKeysymToKeycode(display, XK_Return)] = keycode_to_note[51];
    XGrabKeyboard(display, root_window, false, GrabModeAsync, GrabModeAsync, CurrentTime);

    XAutoRepeatOff(display);
    XSelectInput(display, root_window, KeyPressMask | KeyReleaseMask);
}

void deinitKeys(Display* display) {
    Window root_window = DefaultRootWindow(display);

    XUngrabKeyboard(display, root_window);
    XAutoRepeatOn(display);
}

#define DEFAULT_SHIFT 12
#define UP_SHIFT 12
#define DOWN_SHIFT 12
void reactToKeyEvents(Display* display, int output) {
    bool should_exit = false;
    int shift = DEFAULT_SHIFT;
    if(output == 1) {
        printPianoState();
    } else if(output == 2) {
        printPianoStateSheet();
    }

    while (!should_exit)
    {
        XEvent event;
        XNextEvent(display, &event);
        switch (event.type)
        {
        case KeyPress:
            if(event.xkey.keycode == XKeysymToKeycode(display, XK_Escape)) {
                should_exit = true;
            } else if(event.xkey.keycode == XKeysymToKeycode(display, XK_Shift_L)) {
                shift += UP_SHIFT;
            } else if(event.xkey.keycode == XKeysymToKeycode(display, XK_Control_L)) {
                shift -= DOWN_SHIFT;
            } else {
                pressPianoKey(keycode_to_note[event.xkey.keycode] + BASE_NOTE + shift);
                if(output == 1) {
                    printPianoState();
                } else if(output == 2) {
                    printPianoStateSheet();
                }
            }
            break;
        case KeyRelease:
            if(event.xkey.keycode == XKeysymToKeycode(display, XK_Escape)) {
                should_exit = true;
            } else if(event.xkey.keycode == XKeysymToKeycode(display, XK_Shift_L)) {
                shift -= UP_SHIFT;
            } else if(event.xkey.keycode == XKeysymToKeycode(display, XK_Control_L)) {
                shift += DOWN_SHIFT;
            } else {
                depressPianoKey(keycode_to_note[event.xkey.keycode] + BASE_NOTE + shift);
                if(output == 1) {
                    printPianoState();
                } else if(output == 2) {
                    printPianoStateSheet();
                }
            }
            break;
        }
    }
}

int main(int argc, char** argv) {
    Display* display = XOpenDisplay(NULL);
    int output = 0;

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-onotes") == 0) {
            output = 1;
        }
        if(strcmp(argv[i], "-osheet") == 0) {
            output = 2;
        }
    }

    initPiano();
    initPianoAudio();
    initKeys(display);
    fprintf(stderr, "Ready to play...\n");
    reactToKeyEvents(display, output);
    fprintf(stdout, "\e[J");
    deinitKeys(display);
    deinitPianoAudio();
    deinitPiano();

    XCloseDisplay(display);
    return EXIT_SUCCESS;
}

