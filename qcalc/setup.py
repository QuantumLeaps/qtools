#!/usr/bin/env python
'''
Setup script.
To build qcalc install:
[sudo] python setup.py sdist bdist_wheel
'''

from setuptools import setup

setup(
    name='qcalc',
    version='6.9.2',
    author='Quantum Leaps',
    author_email='info@state-machine.com',
    description='qcalc visualization and monitoring',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://www.state-machine.com/qtools/qcalc.html',
    license='GPL/commercial',
    platforms='any',
    py_modules=['qcalc'],
    entry_points={'console_scripts': ['qcalc = qcalc:main']},
    classifiers=['Development Status :: 5 - Production/Stable',
                 'Intended Audience :: Developers',
                 'Topic :: Software Development :: Embedded Systems',
                 'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
                 'License :: Other/Proprietary License',
                 'Operating System :: Microsoft :: Windows',
                 'Operating System :: POSIX :: Linux',
                 'Operating System :: MacOS :: MacOS X',
                 'Programming Language :: Python',
                 'Programming Language :: Python :: 3'],
)
