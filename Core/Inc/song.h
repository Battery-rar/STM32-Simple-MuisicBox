#ifndef SONG_H
#define SONG_H

typedef enum Note{
	Rest = 0,
	C0,   Cs0, D0,  Ds0, E0,  F0,  Fs0, G0,  Gs0, A0,  As0, B0,
	C1,   Cs1, D1,  Ds1, E1,  F1,  Fs1, G1,  Gs1, A1,  As1, B1,
	C2,   Cs2, D2,  Ds2, E2,  F2,  Fs2, G2,  Gs2, A2,  As2, B2,
	C3,   Cs3, D3,  Ds3, E3,  F3,  Fs3, G3,  Gs3, A3,  As3, B3,
	C4,   Cs4, D4,  Ds4, E4,  F4,  Fs4, G4,  Gs4, A4,  As4, B4,
	C5,   Cs5, D5,  Ds5, E5,  F5,  Fs5, G5,  Gs5, A5,  As5, B5,
	C6,   Cs6, D6,  Ds6, E6,  F6,  Fs6, G6,  Gs6, A6,  As6, B6,
	C7,   Cs7, D7,  Ds7, E7,  F7,  Fs7, G7,  Gs7, A7,  As7, B7,
	C8,
	NOTE_COUNT

}Note;
typedef enum Time{
    _1_16 = 0,_1_8,_1_4,_1_2,_1_1,TIME_COUNT
}Time;

typedef struct Song{
    Note note_pitch;
    Time note_len;
}Song;

// 外部可用的频率表、时值表
extern const float note_freqs[NOTE_COUNT];
extern int note_time[TIME_COUNT];

// 歌曲数组 & 长度
extern Song Start_Music_Sheet[];
extern Song Haruhikage_Sheet[];
extern Song Blossom_Sheet[];
extern Song Cheerleader_Sheet[];
extern const int Start_Music_Note_Num;
extern const int Haruhikage_Note_Num;
extern const int Blossom_Note_Num;
extern const int Cheerleader_Note_Num;
extern const int Start_Music_BPM;
extern const int Haruhikage_BPM;
extern const int Blossom_BPM;
extern const int Cheerleader_BPM;
//extern const int HARUHIKAGE_len;

#endif
