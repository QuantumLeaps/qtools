/**
* \file
* \brief Command-line macros and macros for QS/QSPY
*/

/*! The preprocessor switch to activate the QS software tracing
* instrumentation in the code */
/**
* When defined, Q_SPY activates the QS software tracing instrumentation.
*
* When Q_SPY is not defined, the QS instrumentation in the code does
* not generate any code.
*/
#define Q_SPY

/*! The preprocessor switch to activate the QUTEST unit testing
* instrumentation in the code */
/**
* When defined, Q_UTEST activates the QUTEST unit testing facilities.
*
* When Q_UTEST unit testing is not defined, the unit testing macros
* expand to nothing and do not generate any code.
*/
#define Q_UTEST


