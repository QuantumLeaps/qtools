#!/usr/bin/env python
'''
Setup script.
To build qutest install:
[sudo] python setup.py sdist bdist_wheel
'''

from setuptools import setup

setup(
    name="qutest",
    version="7.1.1",
    author="Quantum Leaps",
    author_email="info@state-machine.com",
    description="QUTest Python scripting support",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://www.state-machine.com/qtools/qutest.html",
    license="GPL/commercial",
    platforms="any",
    py_modules=["qutest"],
    entry_points={"console_scripts": ["qutest = qutest:main"]},
    classifiers=["Development Status :: 5 - Production/Stable",
                 "Intended Audience :: Developers",
                 "Topic :: Software Development :: Embedded Systems",
                 "Topic :: Software Development :: Testing",
                 "Topic :: Software Development :: Testing :: Unit",
                 "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)",
                 "License :: Other/Proprietary License",
                 "Operating System :: Microsoft :: Windows",
                 "Operating System :: POSIX :: Linux",
                 "Operating System :: MacOS :: MacOS X",
                 "Programming Language :: Python",
                 "Programming Language :: Python :: 3",
                 "Programming Language :: C",
                 "Programming Language :: C++"],
)
