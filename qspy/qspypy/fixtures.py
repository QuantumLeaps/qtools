##
# @file
# @ingroup qpspypy
# @brief Test fixtures for PyTest; These fixtures MUST be loaded in your
# conftest.py file to be used

## @cond
#-----------------------------------------------------------------------------
# Last updated for version: 2.0.0
# Last updated on: 2018-08-16
#
# Copyright (c) 2018 Lotus Engineering, LLC
# Copyright (c) 2018 Quantum Leaps, LLC
#
# MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Contact information:
# https://www.state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

import pytest
from qspypy.qutest import qutest_context
import qspypy.config as CONFIG


@pytest.fixture(scope='session')
def session():
    """ test fixture for a complete session (all test files)"""

    # Create the one and only qutest_context used through out the session
    context = qutest_context()

    # Do the context setup
    context.session_setup()

    # Yield context to subfixtures to pass along to tests
    yield context

    # Do the context teardown
    context.session_teardown()


@pytest.fixture(scope='module')
def module(session, request):
    """ Fixture that looks for on_reset() in test module and registers it """

    if hasattr(request.module, "on_reset"):
        on_reset = getattr(request.module, "on_reset")
        session.on_reset_callback = on_reset

    if hasattr(request.module, "on_setup"):
        on_setup = getattr(request.module, "on_setup")
        session.on_setup_callback = on_setup

    if hasattr(request.module, "on_teardown"):
        on_teardown = getattr(request.module, "on_teardown")
        session.on_teardown_callback = on_teardown

    yield session

    session.on_reset_callback = None
    session.on_setup_callback = None
    session.on_teardown_callback = None


@pytest.fixture
def reset(module):
    """ Fixture used for resetting the target, Internal use only"""
    if CONFIG.RESET_TARGET_ON_SETUP:
        module.call_on_reset()
    return module


@pytest.fixture()
def qutest(reset):
    """ Default test fixture for each test function.

    This will reset the target before each test unless
    the RESET_TARGET_ON_SETUP is set to False
    """

    # Setup
    reset.call_on_setup()

    # Run Test
    yield reset

    # Teardown
    reset.call_on_teardown()


@pytest.fixture()
def qutest_noreset(session):
    """ Test fixture for each test function that does NOT reset the target. """

    # Setup
    session.call_on_setup()

    # Run Test
    yield session

    # Teardown
    session.call_on_teardown()
