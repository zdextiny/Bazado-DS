
//{{BLOCK(score_bg)

//======================================================================
//
//	score_bg, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 25 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 800 + 2048 = 3360
//
//	Time-stamp: 2026-08-19, 13:55:37
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_SCORE_BG_H
#define GRIT_SCORE_BG_H

#define score_bgTilesLen 800
extern const unsigned int score_bgTiles[200];

#define score_bgMapLen 2048
extern const unsigned short score_bgMap[1024];

#define score_bgPalLen 512
extern const unsigned short score_bgPal[256];

#endif // GRIT_SCORE_BG_H

//}}BLOCK(score_bg)
