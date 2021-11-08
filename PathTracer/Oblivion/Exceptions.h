#pragma once

#include <exception>
#include <comdef.h>
#include <corecrt_wstring.h>
#include "Conversions.h"

namespace Exceptions {
	class Exception: public std::exception {
	protected:
		std::wstring m_data;
	private:
		mutable std::string m_sdata;
	public:
		virtual const char* what( ) const
		{
			m_sdata = Conversions::ws2s( m_data );

			return m_sdata.c_str( );
		}
	};

	class InitializationException: public Exception {
	public:
		InitializationException( const wchar_t* message, int line, const wchar_t* file )
		{
			m_data = std::wstring( L"Error: \"" ) + std::wstring( message ) + std::wstring( L"\" on line " ) + std::to_wstring( line ) +
				L" in file " + std::wstring( file );
		}
	};

	class MemoryAllocationException: public Exception {
	public:
		MemoryAllocationException( int allocateSize, int line, const wchar_t* file )
		{
			m_data = std::wstring( L"Error: \"Couldn't allocate " ) + std::to_wstring( allocateSize ) +
				std::wstring( L"\" on line " ) + std::to_wstring( line ) +
				L" in file" + std::wstring( file );
		}
	};

	class FailedHResultException: public Exception {
	public:
		FailedHResultException( HRESULT hr )
		{
			_com_error err( hr );
			std::wstring errMessage( err.ErrorMessage( ) );
			m_data = std::wstring( L"Error: code " ) + std::to_wstring( hr ) + L" message: " + errMessage;
		}
	};
}

template <class ... Args>
constexpr void getInitializationError( Args... arg ) {
	return appendToString( arg );
}


#define THROW_ERROR(...) {char message[1024];\
sprintf_s(message, "File: %s, line: %d; Error: ", __FILE__, __LINE__);\
sprintf_s(message + strlen(message),sizeof(message),__VA_ARGS__);\
std::exception error(message);\
throw error;}

#define CSTR_MESSAGE(...) Oblivion::appendToString(__VA_ARGS__).c_str()
#define EVALUATE(expr, ...) if (!(expr)) {\
throw std::exception(Oblivion::appendToString("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__).c_str()); }

#define EVALUATEBK(expr, ...) if (!(expr)) {\
Oblivion::DebugPrintLine("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__);\
break;\
}

#define EVALUATECONT(expr, ...) if (!(expr)) {\
Oblivion::DebugPrintLine("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__);\
continue;\
}

#define EVALUATERET(expr, retValue, ...) if (!(expr)) {\
Oblivion::DebugPrintLine("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__);\
return (retValue);\
}

#define EVALUATERETURN(expr, ...) if (!(expr)) {\
Oblivion::DebugPrintLine("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__);\
return;\
}

#define EVALUATESHOW(expr, ...) if (!(expr)) {\
Oblivion::DebugPrintLine("Error occured in file ", __FILE__, " at line ", __LINE__, ": ", __VA_ARGS__);\
}

#define TRY_PRINT_ERROR(expression) \
try {\
expression;\
} catch (const std::exception& e) {\
Oblivion::DebugPrintLine("Error occured when runnin expression located at line: ", __LINE__, ", file: ", __FILE__, ";\nError message: ", e.what());\
} catch (...) {\
Oblivion::DebugPrintLine("Unexpected error occured when runnin expression located at line: ", __LINE__, ", file: ", __FILE__);\
}\

#define TRY_PRINT_ERROR_AND_MESSAGE(expression, ...) \
try {\
expression;\
} catch (const std::exception& e) {\
char message[1024];\
sprintf_s(message, sizeof(message), __VA_ARGS__); \
Oblivion::DebugPrintLine("Error occured when runnin expression located at line: ", __LINE__, ", file: ", __FILE__, ";\nError message: ", e.what());\
Oblivion::DebugPrintLine("Info message: ", message);\
} catch (...) {\
char message[1024];\
sprintf_s(message, sizeof(message), __VA_ARGS__); \
Oblivion::DebugPrintLine("Unexpected error occured when runnin expression located at line: ", __LINE__, ", file: ", __FILE__);\
Oblivion::DebugPrintLine("Info message: ", message);\
}\

#define TRY_RETURN_VALUE(expression, goodValue, badValue) \
[&] {\
    try {\
        expression;\
    } catch (const std::exception& e) {\
        Oblivion::DebugPrintLine("Error occured when runnin expression located at line: ", __LINE__, ", file: ", __FILE__, "; Error message: ", e.what());\
        return (badValue);\
    } catch (...) {\
        Oblivion::DebugPrintLine("Unexpected error occured when running expression located at line: ", __LINE__, ", file: ", __FILE__);\
        return (badValue);\
    }\
    return (goodValue);\
}()


inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		throw Exceptions::FailedHResultException(hr);
	}
}
