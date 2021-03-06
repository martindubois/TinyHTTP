
// Author   Martin Dubois, P.Eng.
// Product  TinyHTTP
// File     TinyHTTP.cpp

// Includes
/////////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// ===== System =============================================================
#include <sys/socket.h>

// ===== Network ============================================================
#include <netinet/in.h>

// ===== ARPA ===============================================================
#include <arpa/inet.h>

// Constants
/////////////////////////////////////////////////////////////////////////////

#define RESULT_CONTINUE (1)
#define RESULT_STOP     (2)

// Static function declarations
/////////////////////////////////////////////////////////////////////////////

static void CloseSocket        ( int aSocket );
static int  CreateAndBindSocket();

static void DisplayError( const char * aErrorType, const char * aMsg );
static void DisplayError( const char * aErrorType, const char * aFuncCall, int aRet );

static void GetTime( char * aOut );

static int ProcessGet     ( int aSocket, const char * aRequest );
static int ProcessRequest ( int aSocket, const char * aRequest );
static int ProcessRequest ( int aSocket );
static int ProcessRequests( int aSocket );

static unsigned int ReadFile( const char * aFileName, void * aOut, unsigned int aOutSize_byte );

static void SendData  ( int aSocket, const void * aData, unsigned int aSize_byte );
static void SendFile  ( int aSocket, const char * aFileName );
static void SendHeader( int aSocket, unsigned int aStatusCode, const char * aStatusName, unsigned int aSize_byte );

static void Trace( const char        * aMsg    );
static void Trace( const sockaddr_in & aAddrIn );

// Entry point
/////////////////////////////////////////////////////////////////////////////

int main()
{
    Trace( "TinyHTTP - Version 0.0" );

    int lSocket = CreateAndBindSocket();
    if ( 0 > lSocket )
    {
        return __LINE__;
    }

    int lResult = listen( lSocket, 2 );
    if ( 0 == lResult )
    {
        lResult = ProcessRequests( lSocket );
    }
    else
    {
        DisplayError( "FATAL ERROR", "listen( ,  )", lResult );
    }

    CloseSocket( lSocket );

    return lResult;
}

// Static functions
/////////////////////////////////////////////////////////////////////////////

void CloseSocket( int aSocket )
{
    assert( 0 < aSocket );

    Trace( "Closing socket ..." );

    int lRet = close( aSocket );
    if ( 0 != lRet )
    {
        DisplayError( "WARNING", "close(  )", lRet );
    }
}

int CreateAndBindSocket()
{
    Trace( "Creating socket ..." );

    int lResult = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( 0 < lResult )
    {
        Trace( "Binding socket ..." );

        sockaddr_in lAddrIn;

        lAddrIn.sin_family      = AF_INET    ;
        lAddrIn.sin_port        = htons( 80 );
        lAddrIn.sin_addr.s_addr = INADDR_ANY ;

        int lRet = bind( lResult, reinterpret_cast< sockaddr * >( & lAddrIn ), sizeof( lAddrIn ) );
        if ( 0 == lRet )
        {
            Trace( lAddrIn );
        }
        else
        {
            DisplayError( "FATAL ERROR", "bind( , ,  )", lRet );
            CloseSocket( lResult );
            lResult = lRet;
        }
    }
    else
    {
        DisplayError( "FATAL_ERROR", "socket( , ,  )", lResult );
    }

    return lResult;
}

void DisplayError( const char * aErrorType, const char * aMsg )
{
    assert( NULL != aErrorType );
    assert( NULL != aMsg       );

    fprintf( stderr, "%s  %s\n" , aErrorType, aMsg );
}

void DisplayError( const char * aErrorType, const char * aFuncCall, int aRet )
{
    assert( NULL != aErrorType );
    assert( NULL != aFuncCall  );

    fprintf( stderr, "%s  %s  failed\n" , aErrorType, aFuncCall );
    fprintf( stderr, "    Returned %d\n", aRet  );
    fprintf( stderr, "    errno = %d\n" , errno );
}

void GetTime( char * aOut )
{
    assert( NULL != aOut );

    time_t lT;
    
    time_t lTR = time( & lT );
    if ( 0 < lTR )
    {
        struct tm lTM;

        if ( NULL != gmtime_r( & lT, & lTM ) )
        {
            asctime_r( & lTM, aOut );

            aOut[ strlen( aOut ) - 1 ] = '\0';
        }
        else
        {
            DisplayError( "ERROR", "gmtime_r( ,  )  failed" );
            strcpy( aOut, "ERROR  gmtime_r( ,  )  failed" );
        }
    }
    else
    {
        DisplayError( "ERROR", "time(  )", lTR );
        strcpy( aOut, "ERROR  time(  )  failed" );
    }
}

// aRequest [---;R--]
int ProcessGet( int aSocket, const char * aRequest )
{
    assert(    0 <  aSocket  );
    assert( NULL != aRequest );

    Trace( "Processing GET ..." );

    int lResult = RESULT_CONTINUE;

    if ( 0 == strcmp( "/"               , aRequest ) ) { SendFile( aSocket, "/index.htm"       ); }
    if ( 0 == strcmp( "/server_stop.htm", aRequest ) ) { SendFile( aSocket, "/server_stop.htm" ); lResult = RESULT_STOP; }
    else                                               { SendFile( aSocket, aRequest           ); }

    return lResult;
}

// aRequest [---;R--]
int ProcessRequest( int aSocket, const char * aRequest )
{
    assert(    0 <  aSocket  );
    assert( NULL != aRequest );

    char lRequest[ 16 ];
    int  lResult = RESULT_CONTINUE;

    if ( 1 == sscanf( aRequest, "GET %s", lRequest ) )
    {
        lResult = ProcessGet( aSocket, lRequest );
    }
    else
    {
        DisplayError( "ERROR", "Invalid request" );
    }

    return lResult;
}

int ProcessRequest( int aSocket )
{
    assert( 0 < aSocket );

    Trace( "Processing request ...\n" );

    char lBuffer[ 1024 ];

    memset( lBuffer, 0, sizeof( lBuffer ) );

    ssize_t lSize_byte = read( aSocket, lBuffer, sizeof( lBuffer ) );

    Trace( lBuffer );

    int lResult = ProcessRequest( aSocket, lBuffer );

    Trace( "Closing connexion ...\n" );

    CloseSocket( aSocket );

    return lResult;
}

int ProcessRequests( int aSocket )
{
    assert( 0 < aSocket );

    Trace( "Processing requests ...\n" );

    int lResult = RESULT_CONTINUE;

    do
    {
        sockaddr_in lAddr;
        socklen_t   lSize_byte;

        lSize_byte = sizeof( lAddr );

        int lSocket = accept( aSocket, reinterpret_cast< sockaddr * >( & lAddr ), & lSize_byte );
        if ( 0 < lSocket )
        {
            lResult = ProcessRequest( lSocket );
        }
        else
        {
            DisplayError( "ERROR", "accept( , ,  )", lSocket );

            switch ( errno )
            {
            case EINVAL :
                lResult = - errno;
                break;
            }
        }
    }
    while ( RESULT_CONTINUE == lResult );

    return lResult;
}

// aFileName [---;R--]
// aOut      [---;-W-]
unsigned int ReadFile( const char * aFileName, void * aOut, unsigned int aOutSize_byte )
{
    assert( NULL != aFileName     );
    assert( NULL != aOut          );
    assert(    0 <  aOutSize_byte );

    unsigned int lResult_byte = 0;

    FILE * lFile = fopen( aFileName, "r" );
    if ( NULL == lFile )
    {
        DisplayError( "ERROR", "Invalid file name" );
    }
    else
    {
        int lRet = fread( aOut, 1, aOutSize_byte, lFile );
        if ( 0 > lRet )
        {
            DisplayError( "ERROR", "fread( , , ,  )", lRet );
        }
        else
        {
            if ( aOutSize_byte == lRet )
            {
                DisplayError( "WARNING", "The file may be longer than the internal buffer\n" );
            }

            lResult_byte = lRet;
        }

        lRet = fclose( lFile );
        if ( 0 != lRet )
        {
            DisplayError( "WARNING", "fclose(  )", lRet );
        }
    }

    return lResult_byte;
}

// aData [---;R--]
void SendData( int aSocket, const char * aData, unsigned int aSize_byte )
{
    assert( 0    <  aSocket    );
    assert( NULL != aData      );
    assert(    0 <  aSize_byte );

    Trace( "Sending data ..." );

    ssize_t lSent_byte = write( aSocket, aData, aSize_byte );
    if ( aSize_byte != lSent_byte )
    {
        DisplayError( "ERROR", "write( , ,  )", lSent_byte );
    }
}

// aFileName [---;R--]
void SendFile( int aSocket, const char * aFileName )
{
    assert(    0 != aSocket   );
    assert( NULL != aFileName );

    Trace( "Sending file ..." );

    char lFileName[ 16 ];

    sprintf( lFileName, "../Data%s", aFileName );

    char lBuffer[ 16 * 1024 ];

    unsigned int lSize_byte = ReadFile( lFileName, lBuffer, sizeof( lBuffer ) );
    if ( 0 < lSize_byte )
    {
        SendHeader( aSocket, 200, "OK", lSize_byte );
        SendData  ( aSocket, lBuffer, lSize_byte );
    }
    else
    {
        SendHeader( aSocket, 404, "ERROR", 0 );
    }
}

// aStatusName [---;R--]
void SendHeader( int aSocket, unsigned int aStatusCode, const char * aStatusName, unsigned int aSize_byte )
{
    assert(    0 <  aSocket     );
    assert( NULL != aStatusName );

    Trace( "Sending header ..." );

    char lTimeStr[ 32 ];

    GetTime( lTimeStr );

    char lBuffer[ 128 ];

    sprintf( lBuffer,
        "HTTP/1.1 %u %s\t\n"
        "Date: %s GMT\r\n"
        "Server: TinyHTTP\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: text/html\r\n"
        "\r\n",
        aStatusCode, aStatusName,
        lTimeStr,
        aSize_byte );

    Trace( lBuffer );

    SendData( aSocket, lBuffer, strlen( lBuffer ) );
}

// aMsg [---;R--]
void Trace( const char * aMsg )
{
    assert( NULL != aMsg );

    printf( "%s\n", aMsg );
}

// aAddrIn [---;R--]
void Trace( const sockaddr_in & aAddrIn )
{
    assert( NULL != ( & aAddrIn ) );

    printf( "0x%08x:%u (%u)\n", aAddrIn.sin_addr.s_addr, ntohs( aAddrIn.sin_port ), aAddrIn.sin_family );
}
