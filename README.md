midi-key-guesser
================

Guess the key of a piece of music from its MIDI file

See also @tmcw's https://beta.observablehq.com/@tmcw/determining-the-key-of-bwv1001-1st-movement-adagio

Example:

```
$ curl -L -O https://archive.org/download/BachsSoloViolinWorks/Zipped_MIDI_BWV1001-1006_DJG.zip
$ mkdir mozart
$ (cd mozart; unzip ../Zipped_MIDI_BWV1001-1006_DJG.zip)

$ make
$ ./midi mozart/BWV1001/*.mid
0.722 0.441 0.629 0.785 0.328 0.788 0.381 0.697 0.619 0.439 0.843 0.330 Bb mozart/BWV1001/vs1-1ada.mid
0.764 0.442 0.628 0.751 0.321 0.838 0.357 0.696 0.561 0.486 0.842 0.313 Bb mozart/BWV1001/vs1-2fug.mid
0.691 0.596 0.421 0.860 0.268 0.842 0.458 0.572 0.700 0.312 0.941 0.340 Bb mozart/BWV1001/vs1-3sic.mid
0.699 0.512 0.558 0.755 0.343 0.820 0.405 0.664 0.603 0.431 0.860 0.350 Bb mozart/BWV1001/vs1-4prs.mid
```

Guesses that these are in Bb major/G minor.
