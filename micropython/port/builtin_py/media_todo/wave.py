import builtins

__all__ = ["open", "openfp", "Error"]

class Error(Exception):
    pass

WAVE_FORMAT_PCM = 0x0001

_array_fmts = None, 'b', 'h', None, 'i'

#import audioop
import struct
import sys
from media.chunk import Chunk
from collections import namedtuple

_wave_params = namedtuple('_wave_params',
                     'nchannels sampwidth framerate nframes comptype compname')

class Wave_read:

    def initfp(self, file):
        self._convert = None
        self._soundpos = 0
        self._file = Chunk(file, bigendian = 0)
        if self._file.getname() != b'RIFF':
            raise Error('file does not start with RIFF id')
        if self._file.read(4) != b'WAVE':
            raise Error('not a WAVE file')
        self._fmt_chunk_read = 0
        self._data_chunk = None
        while 1:
            self._data_seek_needed = 1
            try:
                chunk = Chunk(self._file, bigendian = 0)
            except EOFError:
                break
            chunkname = chunk.getname()
            if chunkname == b'fmt ':
                self._read_fmt_chunk(chunk)
                self._fmt_chunk_read = 1
            elif chunkname == b'data':
                if not self._fmt_chunk_read:
                    raise Error('data chunk before fmt chunk')
                self._data_chunk = chunk
                self._nframes = chunk.chunksize // self._framesize
                self._data_seek_needed = 0
                break
            chunk.skip()
        if not self._fmt_chunk_read or not self._data_chunk:
            raise Error('fmt chunk and/or data chunk missing')

    def __init__(self, f):
        self._i_opened_the_file = None
        if isinstance(f, str):
            f = builtins.open(f, 'rb')
            self._i_opened_the_file = f
        # else, assume it is an open file object already
        try:
            self.initfp(f)
        except:
            if self._i_opened_the_file:
                f.close()
            raise

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    #
    # User visible methods.
    #
    def getfp(self):
        return self._file

    def rewind(self):
        self._data_seek_needed = 1
        self._soundpos = 0

    def close(self):
        if self._i_opened_the_file:
            self._i_opened_the_file.close()
            self._i_opened_the_file = None
        self._file = None

    def tell(self):
        return self._soundpos

    def get_channels(self):
        return self._nchannels

    def get_frames(self):
        return self._nframes

    def get_sampwidth(self):
        return self._sampwidth

    def get_framerate(self):
        return self._framerate

    def get_comptype(self):
        return self._comptype

    def get_compname(self):
        return self._compname

    def get_params(self):
        return _wave_params(self.get_channels(), self.get_sampwidth(),
                       self.get_framerate(), self.get_frames(),
                       self.get_comptype(), self.get_compname())

    def get_markers(self):
        return None

    def get_mark(self, id):
        raise Error('no marks')

    def setpos(self, pos):
        if pos < 0 or pos > self._nframes:
            raise Error('position not in range')
        self._soundpos = pos
        self._data_seek_needed = 1

    def read_frames(self, nframes):
        if self._data_seek_needed:
            self._data_chunk.seek(0, 0)
            pos = self._soundpos * self._framesize
            if pos:
                self._data_chunk.seek(pos, 0)
            self._data_seek_needed = 0
        if nframes == 0:
            return b''
        data = self._data_chunk.read(nframes * self._framesize)
        if self._sampwidth != 1 and sys.byteorder == 'big':
            data = audioop.byteswap(data, self._sampwidth)
        if self._convert and data:
            data = self._convert(data)
        self._soundpos = self._soundpos + len(data) // (self._nchannels * self._sampwidth)
        return data

    #
    # Internal methods.
    #

    def _read_fmt_chunk(self, chunk):
        wFormatTag, self._nchannels, self._framerate, dwAvgBytesPerSec, wBlockAlign = struct.unpack('<HHLLH', chunk.read(14))
        if wFormatTag == WAVE_FORMAT_PCM:
            sampwidth = struct.unpack('<H', chunk.read(2))[0]
            self._sampwidth = (sampwidth + 7) // 8
        else:
            raise Error('unknown format: %r' % (wFormatTag,))
        self._framesize = self._nchannels * self._sampwidth
        self._comptype = 'NONE'
        self._compname = 'not compressed'

class Wave_write:

    def __init__(self, f):
        self._i_opened_the_file = None
        if isinstance(f, str):
            f = builtins.open(f, 'wb')
            self._i_opened_the_file = f
        try:
            self.initfp(f)
        except:
            if self._i_opened_the_file:
                f.close()
            raise

    def initfp(self, file):
        self._file = file
        self._convert = None
        self._nchannels = 0
        self._sampwidth = 0
        self._framerate = 0
        self._nframes = 0
        self._nframeswritten = 0
        self._datawritten = 0
        self._datalength = 0
        self._headerwritten = False

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    #
    # User visible methods.
    #
    def set_channels(self, nchannels):
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        if nchannels < 1:
            raise Error('bad # of channels')
        self._nchannels = nchannels

    def get_channels(self):
        if not self._nchannels:
            raise Error('number of channels not set')
        return self._nchannels

    def set_sampwidth(self, sampwidth):
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        if sampwidth < 1 or sampwidth > 4:
            raise Error('bad sample width')
        self._sampwidth = sampwidth

    def get_sampwidth(self):
        if not self._sampwidth:
            raise Error('sample width not set')
        return self._sampwidth

    def set_framerate(self, framerate):
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        if framerate <= 0:
            raise Error('bad frame rate')
        self._framerate = int(round(framerate))

    def get_framerate(self):
        if not self._framerate:
            raise Error('frame rate not set')
        return self._framerate

    def set_frames(self, nframes):
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        self._nframes = nframes

    def get_frames(self):
        return self._nframeswritten

    def set_comptype(self, comptype, compname):
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        if comptype not in ('NONE',):
            raise Error('unsupported compression type')
        self._comptype = comptype
        self._compname = compname

    def get_comptype(self):
        return self._comptype

    def get_compname(self):
        return self._compname

    def set_params(self, params):
        nchannels, sampwidth, framerate, nframes, comptype, compname = params
        if self._datawritten:
            raise Error('cannot change parameters after starting to write')
        self.set_channels(nchannels)
        self.set_sampwidth(sampwidth)
        self.set_framerate(framerate)
        self.set_frames(nframes)
        self.set_comptype(comptype, compname)

    def get_params(self):
        if not self._nchannels or not self._sampwidth or not self._framerate:
            raise Error('not all parameters set')
        return _wave_params(self._nchannels, self._sampwidth, self._framerate,
              self._nframes, self._comptype, self._compname)

    def set_mark(self, id, pos, name):
        raise Error('set_mark() not supported')

    def get_mark(self, id):
        raise Error('no marks')

    def get_markers(self):
        return None

    def tell(self):
        return self._nframeswritten

    def write_frames_raw(self, data):
        if not isinstance(data, (bytes, bytearray)):
            data = memoryview(data).cast('B')
        self._ensure_header_written(len(data))
        nframes = len(data) // (self._sampwidth * self._nchannels)
        if self._convert:
            data = self._convert(data)
        if self._sampwidth != 1 and sys.byteorder == 'big':
            data = audioop.byteswap(data, self._sampwidth)
        self._file.write(data)
        self._datawritten += len(data)
        self._nframeswritten = self._nframeswritten + nframes

    def write_frames(self, data):
        self.write_frames_raw(data)
        if self._datalength != self._datawritten:
            self._patchheader()

    def close(self):
        if self._file:
            try:
                self._ensure_header_written(0)
                if self._datalength != self._datawritten:
                    self._patchheader()
                self._file.flush()
            finally:
                self._file = None
        if self._i_opened_the_file:
            self._i_opened_the_file.close()
            self._i_opened_the_file = None

    #
    # Internal methods.
    #

    def _ensure_header_written(self, datasize):
        if not self._headerwritten:
            if not self._nchannels:
                raise Error('# channels not specified')
            if not self._sampwidth:
                raise Error('sample width not specified')
            if not self._framerate:
                raise Error('sampling rate not specified')
            self._write_header(datasize)

    def _write_header(self, initlength):
        assert not self._headerwritten
        self._file.write(b'RIFF')
        if not self._nframes:
            self._nframes = initlength // (self._nchannels * self._sampwidth)
        self._datalength = self._nframes * self._nchannels * self._sampwidth
        try:
            self._form_length_pos = self._file.tell()
        except (AttributeError, OSError):
            self._form_length_pos = None
        self._file.write(struct.pack('<L4s4sLHHLLHH4s',
            36 + self._datalength, b'WAVE', b'fmt ', 16,
            WAVE_FORMAT_PCM, self._nchannels, self._framerate,
            self._nchannels * self._framerate * self._sampwidth,
            self._nchannels * self._sampwidth,
            self._sampwidth * 8, b'data'))
        if self._form_length_pos is not None:
            self._data_length_pos = self._file.tell()
        self._file.write(struct.pack('<L', self._datalength))
        self._headerwritten = True

    def _patchheader(self):
        assert self._headerwritten
        if self._datawritten == self._datalength:
            return
        curpos = self._file.tell()
        self._file.seek(self._form_length_pos, 0)
        self._file.write(struct.pack('<L', 36 + self._datawritten))
        self._file.seek(self._data_length_pos, 0)
        self._file.write(struct.pack('<L', self._datawritten))
        self._file.seek(curpos, 0)
        self._datalength = self._datawritten

def open(f, mode=None):
    if mode is None:
        if hasattr(f, 'mode'):
            mode = f.mode
        else:
            mode = 'rb'
    if mode in ('r', 'rb'):
        return Wave_read(f)
    elif mode in ('w', 'wb'):
        return Wave_write(f)
    else:
        raise Error("mode must be 'r', 'rb', 'w', or 'wb'")

openfp = open # B/W compatibility
