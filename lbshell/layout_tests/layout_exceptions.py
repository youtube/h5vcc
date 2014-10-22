"""Exceptions for layout tests."""


class Error(Exception):
  """Base class for exceptions in this module."""
  pass


class TestFileError(Error):
  """Exception for problems related to test files (i.e. if they are missing).

  Attributes:
    strerror -- Explanation of the error
  """

  def __init__(self, msg):
    self.strerror = msg
    super(TestFileError, self).__init__()


class TestClientError(Error):
  """Exception for problems setting up or running the test client.

  Attributes:
    strerror -- Explanation of the error
  """

  def __init__(self, msg):
    self.strerror = msg
    super(TestClientError, self).__init__()


class TestTimeoutError(Error):
  """Exception for timing out when waiting on and returning test output.

  Attributes:
    test -- The name of the test that the error occurred on
  """

  def __init__(self, test):
    self.test = test
    super(TestTimeoutError, self).__init__()
