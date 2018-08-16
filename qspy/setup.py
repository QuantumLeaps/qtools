# Setup script for the "qspypy" package
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

import setuptools

with open("qspypy/README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="qspypy",
    version="2.0.0",
    author="Lotus Engineering, LLC/Quantum Leaps, LLC",
    author_email="info@state-machine.com",
    description="Python support for QUTest scripts",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://www.state-machine.com/qtools/qutest.html",
    packages=['qspypy'],
    classifiers=(
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ),
    keywords = 'qtools qutest qp qpc qpcpp',
    install_requires = ['pytest>=3.6.1'],
    entry_points = {
        'console_scripts': ['qutest=qspypy.qutest:main'],
    }
)