"""Diff images."""

import subprocess


class ImageMagickError(Exception):

  def __init__(self, msg):
    self.strerror = msg
    Exception.__init__()

  def __str__(self):
    return 'ImageMagick error -- ' + self.strerror


def DiffImageFiles(img_file_1, img_file_2, diff_file):
  """Diff two images.

  Open img_file_1 and img_file_2, compare the pixels, and write the diff
  output to diff_file

  Args:
    img_file_1: First image to compare.
    img_file_2: Second image to compare.
    diff_file: File to write diff output to.

  Returns:
    integer describing number of differing pixels.

  Raises:
    ImageMagickError: ImageMagick not found.
  """

  args = ['compare', '-fuzz', '5%', '-metric', 'AE', '-compose', 'Src',
          '-highlight-color', 'White', '-lowlight-color', 'Black',
          img_file_1, img_file_2, diff_file]

  try:
    im_proc = subprocess.Popen(args, stderr=subprocess.PIPE)
    result = im_proc.wait()
  except OSError:
    raise ImageMagickError('Error calling compare. Is ImageMagick in the path?')

  if result == 0:
    # Success, and the number of differing pixels should be printed to
    # stdout
    return int(im_proc.stderr.read())
  else:
    # Error, and the error message should be printed to stdout
    raise ImageMagickError(im_proc.stderr.read())
