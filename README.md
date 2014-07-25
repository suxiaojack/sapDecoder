sapDecoder
========

Spatial acoustics parameter decoder, which can decode stereo audio to 5.1 surround or virtual sournd for surrounding playback using 3D sound engine based on MIT HRTF


Platform
========
Windows 7 32/64<br>
Linux 3.0 later<br>
Android 4.0 later<br>


Requirement
========

libtsp : http://www-mmsp.ece.mcgill.ca/documents/Software/Packages/libtsp/libtsp.html<br>
flann  : https://github.com/mariusmuja/flann<br>
vorbis : http://www.vorbis.com<br>

Compile
========
Windows: 

  cd win32<br>
  vs2010 open<br>
  
Linux:

  make clean<br>
  make<br>

Android:

  cp Makefile.android Makefile<br>
  make clean<br>
  make<br>
