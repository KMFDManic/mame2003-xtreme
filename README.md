# MAME2003-Xtreme
KMFDManic Fork of MAME Coding that does not at all adhere to accuracy, and instead favors Performance/Speed above all else, at the expense of stability!  Meant for low-spec platforms!

## Xtreme Custom OST Support
Easily the best feature of 2003 Xtreme!  Big Blue Frontend initially started the trend with Final Fight in Current MAME.  Gpstar81 C++ backported the necessary coding into Xtreme.  It then branched out to support several others.  Mahoneyt944, Grant2258, Arcadez2003 have been phenomenally helpful in making the support exponentially better, over time!  Mahoney has separated and consolidated support for external samples outside of the primary arcade drivers, lending credence to less clutter and "crossfires".  Grant has implemented a myriad of bug/accuracy fixes and improvements.  

I, KMFDManic have worked in the Xtreme Magic to maintain integrity with overall performance/speed/memory usage, considering Custom OSTs are potential memory hogs!  Mathematically, RAM usage can be trickery, with memory load maxing out at 80MB under normal circumstances (core+retroarch+zipped samples).  I have added fixes to help offset this for lower spec platforms.  Kurrono has also helped carry the torch, with his extensive music hook hacking skills.  You would have surely come across many of his previous successes in the Mesen and MSU-1 Communities!  Every project he has worked on it now a mainstay in my permanent gaming playlist:)

**The currently supported Custom OSTs in 2003 Xtreme include:**
- Bad Dudes
- Bionic Commando
- Cave Ninja AKA Joe & Mac
- Double Dragon
- Final Fight
- Ghosts 'n Goblins
- Haunted Castle
- Ikari Warriors
- Moonwalker
- Mortal Kombat II
- NBA Jam
- OutRun
- RoboCop
- Shinobi
- Street Fighter I
- Street Fighter II
- Ultraman

**It is a bit technical, the creation of such Custom OSTs**

Those interested in helping flesh out the Sample Sets for Support with MAME 2003 Xtreme/Plus, are welcome to reply in the Custom OST Testing Grounds Issue on my 2003 Xtreme MAME Repo.  There are MANY more Custom OSTs to be added.  But, they also need TLC, as far as having magnificent, non-broken record flair...for replayability...to have hopes of remotely matching the awesomeness of the originals!  Several Samples are already completed.  Others are coded in, but need Samples finished.  No matter the case, we will get there:)

## ROMset compatibility with MAME 2003 Xtreme
**The MAME 2003 Xtreme core accepts MAME 0.78 ROMsets.** Each version of an arcade emulator must be used with ROMs that have the same exact version number. For example, MAME 0.37b5 ROMsets are required by the MAME 2000 emulator, but MAME 0.37b5 sets will not work correctly with the MAME 2003 or MAME 2010 emulator cores. Those cores require MAME 0.78 and MAME 0.139 ROM sets, respectively.

File-not-found errors are the result of a ROMset that is wrong or incomplete, including if you're trying to run a "Split" clone .zip without the parent .zip present. Non-Merged MAME0.78 ROMsets do not require parent .zips to be present.

## Multiple lightgun, steering wheel, and "spinner/dial" devices
MAME 2003 Xtreme has support for multiple mice or touch devices in games that support trackballs, etc.

MAME 2003 Xtreme also supports one or two spinners/dials via the "Share 2 player dial controls across one X/Y device" core option.

## Compiling for the Wii
* To run on Wii's memory constraints some drivers in src/driver.c must be removed.

## Directories
* Some MAME games require data from an internal hard drive, CD-ROM, laserdisk, or other media in order to be emulated -- those forms of media are packaged as CHD files. CHD files should be copied to subfolders within the folder where the MAME ROM zips have been installed. e.g.:
```
/libretro content dir/blitz/blitz.chd
```
* Some games require an additional zip file with recorded sounds or music in order for audio to work correctly. Audio 'sample' files should be placed in subdirectories within `/libretro system dir/mame2003-xtreme/` e.g.:
```
/libretro system dir/mame2003-xtreme/samples/
```
* High score, cheat, and history metadata files should be moved from github's [`/KMFDManic/mame2003-xtreme/tree/master/metadata`](https://github.com/KMFDManic/mame2003-xtreme/tree/master/metadata) and placed within `/libretro system dir/mame2003-xtreme/` e.g.:
```
/libretro system dir/mame2003-xtreme/hiscore.dat
/libretro system dir/mame2003-xtreme/cheat.dat
/libretro system dir/mame2003-xtreme/history.dat
```
* User-generated content is placed in sub-directories within `/libretro savefile dir/mame2003-xtreme/` e.g.:
```
/libretro savefile dir/mame2003-xtreme/diff/
/libretro savefile dir/mame2003-xtreme/nvram/
/libretro savefile dir/mame2003-xtreme/hi/
/libretro savefile dir/mame2003-xtreme/cfg/
/libretro savefile dir/mame2003-xtreme/inp/
/libretro savefile dir/mame2003-xtreme/memcard/
/libretro savefile dir/mame2003-xtreme/snap/
```

### Core options
* **TurboBoost** (0-20; Automatic, Aggressive, etc)
  Smart FrameSkip, which exploits timing in such a way to better help games, such as KI, Carnevil, Virtua Fighter, etc!
* **Reverse OverClock** (1-200; 84 Default)
  Reverse CPU Scaling helps with lower spec hardwares, such as the Mini Classics, RetroPie 2, 3, etc!
* **DCS Speedhack** (enabled/disabled)
  Speedhack for the Midway sound hardware used in Mortal Kombat 2, 3 and others. Improves performance in these games.
* **Skip Disclaimer** (enabled/disabled)
  Skips the 'nag-screen'.
* **Skip Warnings** (disabled/enabled)
  Skips the warning screen shown before games with incomplete emulation.
* **Samples** (enabled/disabled)
  Requires valid sample zips.
* **Sample Rate (KHz)** (11025-48000)
  Lowering may improve performance on weaker devices
* **Cheats** (disabled/enabled)
  Requires a valid cheat.dat file.
* **Share 2 player dial controls across one X/Y device** (disabled/enabled)
  Some dial/spinner hardware are actually one device with one axis for each player. This supports that setup, by breaking down the normal mouse x/y into two seperate inputs.
* **Mouse Device** (mouse/pointer/disabled)
  Switch between mouse (e.g. hardware mouse, trackball, etc), pointer (touchpad, touchscreen, lightgun, etc), or disabled.
* **TATE Mode** (disabled/enabled)
  Enable if rotating display for vertically oriented games (Pac-Man, Galaga, etc). Requires `video_allow_rotate = "false"` cfg setting in RetroArch.

## Additional configuration information:
 * Official RetroArch documentation for the mame2003-libretro core are published at [buildbot.libretro.com/docs](https://buildbot.libretro.com/docs/). They can be [edited and improved by users via github.com](https://github.com/libretro/docs/blob/master/docs/library/mame2003.md).
 * The RetroArch documentation owes much to RetroPie's [detailed MAME 2003 configuration wiki](https://github.com/RetroPie/RetroPie-Setup/wiki/lr-mame2003) which can be adapted for use in other environments.
 
### Development reference links:
 * [MAME: Benchmarks, Useful Code, Bug Fixes, Known Issues](http://www.anthrofox.org/code/mame/index.html) at anthrofox.org
 * [diff file which records efforts taken to address the unaligned memory issue](https://code.oregonstate.edu/svn/dsp_bd/uclinux-dist/trunk/user/games/xmame/xmame-0.106/src/unix/contrib/patches/word-align-patch)
 * [Directory of xmame diffs](http://web.archive.org/web/20090718202532/http://www.filewatcher.com/b/ftp/ftp.zenez.com/pub/mame/xmame.0.0.html) - Offline as of March 2017

### Very Special Thanks:
 * Arcadez2003, Grant2258, Gpstar81, Mahoneyt944, Wilstorm, dinkc64, barbudreadmon, and all other Arcade Devs!
 
