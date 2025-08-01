/*******************************************************************************
 * @file errno.h
 *
 * @see errno.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/10/2024
 *
 * @version 1.0
 *
 * @brief User errno library.
 *
 * @details User errno library. Defines the errno values and errno getter.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_ERRNO_H_
#define __LIB_ERRNO_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Not super-user */
#define	EPERM 1
/** @brief No such file or directory */
#define	ENOENT 2
/** @brief No such process */
#define	ESRCH 3
/** @brief Interrupted system call */
#define	EINTR 4
/** @brief I/O error */
#define	EIO 5
/** @brief No such device or address */
#define	ENXIO 6
/** @brief Arg list too long */
#define	E2BIG 7
/** @brief Exec format error */
#define	ENOEXEC 8
/** @brief Bad file number */
#define	EBADF 9
/** @brief No children */
#define	ECHILD 10
/** @brief No more processes */
#define	EAGAIN 11
/** @brief Not enough core */
#define	ENOMEM 12
/** @brief Permission denied */
#define	EACCES 13
/** @brief Bad address */
#define	EFAULT 14
/** @brief Block device required */
#define	ENOTBLK 15
/** @brief Mount device busy */
#define	EBUSY 16
/** @brief File exists */
#define	EEXIST 17
/** @brief Cross-device link */
#define	EXDEV 18
/** @brief No such device */
#define	ENODEV 19
/** @brief Not a directory */
#define	ENOTDIR 20
/** @brief Is a directory */
#define	EISDIR 21
/** @brief Invalid argument */
#define	EINVAL 22
/** @brief Too many open files in system */
#define	ENFILE 23
/** @brief Too many open files */
#define	EMFILE 24
/** @brief Not a typewriter */
#define	ENOTTY 25
/** @brief Text file busy */
#define	ETXTBSY 26
/** @brief File too large */
#define	EFBIG 27
/** @brief No space left on device */
#define	ENOSPC 28
/** @brief Illegal seek */
#define	ESPIPE 29
/** @brief Read only file system */
#define	EROFS 30
/** @brief Too many links */
#define	EMLINK 31
/** @brief Broken pipe */
#define	EPIPE 32
/** @brief Math arg out of domain of func */
#define	EDOM 33
/** @brief Math result not representable */
#define	ERANGE 34
/** @brief No message of desired type */
#define	ENOMSG 35
/** @brief Identifier removed */
#define	EIDRM 36
/** @brief Channel number out of range */
#define	ECHRNG 37
/** @brief Level 2 not synchronized */
#define	EL2NSYNC 38
/** @brief Level 3 halted */
#define	EL3HLT 39
/** @brief Level 3 reset */
#define	EL3RST 40
/** @brief Link number out of range */
#define	ELNRNG 41
/** @brief Protocol driver not attached */
#define	EUNATCH 42
/** @brief No CSI structure available */
#define	ENOCSI 43
/** @brief Level 2 halted */
#define	EL2HLT 44
/** @brief Deadlock condition */
#define	EDEADLK 45
/** @brief No record locks available */
#define	ENOLCK 46
/** @brief Invalid exchange */
#define EBADE 50
/** @brief Invalid request descriptor */
#define EBADR 51
/** @brief Exchange full */
#define EXFULL 52
/** @brief No anode */
#define ENOANO 53
/** @brief Invalid request code */
#define EBADRQC 54
/** @brief Invalid slot */
#define EBADSLT 55
/** @brief File locking deadlock error */
#define EDEADLOCK 56
/** @brief Bad font file fmt */
#define EBFONT 57
/** @brief Device not a stream */
#define ENOSTR 60
/** @brief No data (for no delay io) */
#define ENODATA 61
/** @brief Timer expired */
#define ETIME 62
/** @brief Out of streams resources */
#define ENOSR 63
/** @brief Machine is not on the network */
#define ENONET 64
/** @brief Package not installed */
#define ENOPKG 65
/** @brief The object is remote */
#define EREMOTE 66
/** @brief The link has been severed */
#define ENOLINK 67
/** @brief Advertise error */
#define EADV 68
/** @brief Srmount error */
#define ESRMNT 69
/** @brief Communication error on send */
#define	ECOMM 70
/** @brief Protocol error */
#define EPROTO 71
/** @brief Multihop attempted */
#define	EMULTIHOP 74
/** @brief Inode is remote (not really error) */
#define	ELBIN 75
/** @brief Cross mount point (not really error) */
#define	EDOTDOT 76
/** @brief Trying to read unreadable message */
#define EBADMSG 77
/** @brief Inappropriate file type or format */
#define EFTYPE 79
/** @brief Given log. name not unique */
#define ENOTUNIQ 80
/** @brief f.d. invalid for this operation */
#define EBADFD 81
/** @brief Remote address changed */
#define EREMCHG 82
/** @brief Can't access a needed shared lib */
#define ELIBACC 83
/** @brief Accessing a corrupted shared lib */
#define ELIBBAD 84
/** @brief .lib section in a.out corrupted */
#define ELIBSCN 85
/** @brief Attempting to link in too many libs */
#define ELIBMAX 86
/** @brief Attempting to exec a shared library */
#define ELIBEXEC 87
/** @brief Function not implemented */
#define ENOSYS 88
/** @brief No more files */
#define ENMFILE 89
/** @brief Directory not empty */
#define ENOTEMPTY 90
/** @brief File or path name too long */
#define ENAMETOOLONG 91
/** @brief Too many symbolic links */
#define ELOOP 92
/** @brief Operation not supported on transport endpoint */
#define EOPNOTSUPP 95
/** @brief Protocol family not supported */
#define EPFNOSUPPORT 96
/** @brief Connection reset by peer */
#define ECONNRESET 104
/** @brief No buffer space available */
#define ENOBUFS 105
/** @brief Address family not supported by protocol family */
#define EAFNOSUPPORT 106
/** @brief Protocol wrong type for socket */
#define EPROTOTYPE 107
/** @brief Socket operation on non-socket */
#define ENOTSOCK 108
/** @brief Protocol not available */
#define ENOPROTOOPT 109
/** @brief Can't send after socket shutdown */
#define ESHUTDOWN 110
/** @brief Connection refused */
#define ECONNREFUSED 111
/** @brief Address already in use */
#define EADDRINUSE 112
/** @brief Connection aborted */
#define ECONNABORTED 113
/** @brief Network is unreachable */
#define ENETUNREACH 114
/** @brief Network interface is not configured */
#define ENETDOWN 115
/** @brief Connection timed out */
#define ETIMEDOUT 116
/** @brief Host is down */
#define EHOSTDOWN 117
/** @brief Host is unreachable */
#define EHOSTUNREACH 118
/** @brief Connection already in progress */
#define EINPROGRESS 119
/** @brief Socket already connected */
#define EALREADY 120
/** @brief Destination address required */
#define EDESTADDRREQ 121
/** @brief Message too long */
#define EMSGSIZE 122
/** @brief Unknown protocol */
#define EPROTONOSUPPORT 123
/** @brief Socket type not supported */
#define ESOCKTNOSUPPORT 124
/** @brief Address not available */
#define EADDRNOTAVAIL 125
/** @brief Net reset */
#define ENETRESET 126
/** @brief Socket is already connected */
#define EISCONN 127
/** @brief Socket is not connected */
#define ENOTCONN 128
/** @brief Too many references */
#define ETOOMANYREFS 129
/**
 * @brief Per-user limit on new process would be exceeded by an attempted
 * fork.
 */
#define EPROCLIM 130
/**
 * @brief
 * The file quota system is confused because there are too many users.
 */
#define EUSERS 131
/** @brief The user's disk quota was exceeded. */
#define EDQUOT 132
/** @brief Stale NFS file handle. */
#define ESTALE 133
/** @brief Not supported */
#define ENOTSUP 134
/** @brief No medium (in tape drive) */
#define ENOMEDIUM 135
/** @brief No such host or network path */
#define ENOSHARE 136
/** @brief Filename exists with different case */
#define ECASECLASH 137
/**
 * @brief While decoding a multibyte character the function came along an
 * invalid or an incomplete sequence of bytes or the given wide character is
 * invalid.
 */
#define EILSEQ 138
/** @brief Value too large for defined data type */
#define EOVERFLOW 139
/** @brief Operation would block */
#define EWOULDBLOCK EAGAIN

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/* None */

#endif /* #ifndef __LIB_ERRNO_H_ */

/************************************ EOF *************************************/