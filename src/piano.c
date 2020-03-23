
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
    char name_buffer[256];
    snprintf(name_buffer, 256, "samples/%i.wav", note);
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
    fprintf(stdout, "\e[u\e[K\e[s");
    char str_buffer[10];
    for(int ps = 0; ps < NUMBER_OF_NOTES; ps++) {
        if(piano[ps].playing_state.pressed) {
            noteToString(ps + BASE_NOTE, str_buffer);
            fprintf(stdout, "%s ", str_buffer);
        }
    }
    fprintf(stdout, "\n");
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
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 36,
    94, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
};

void initKeys(Display* display) {
    Window root_window = DefaultRootWindow(display);

    for (int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        XGrabKey(display, keys[i], 0, root_window, false, GrabModeAsync, GrabModeAsync);
        keycode_to_note[keys[i]] = i;
    }
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
void reactToKeyEvents(Display* display) {
    bool should_exit = false;
    int shift = DEFAULT_SHIFT;

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
                printPianoState();
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
                printPianoState();
            }
            break;
        }
    }
}

int main() {
    Display* display = XOpenDisplay(NULL);

    initPiano();
    initPianoAudio();
    initKeys(display);
    fprintf(stderr, "Ready to play...\n");
    fprintf(stdout, "\e[s");
    reactToKeyEvents(display);
    deinitKeys(display);
    deinitPianoAudio();
    deinitPiano();

    XCloseDisplay(display);
    return EXIT_SUCCESS;
}

