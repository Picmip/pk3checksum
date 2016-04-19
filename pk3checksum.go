package main

//#include "md5.h"
//#include "files.h"
import "C"
import "fmt"
import "os"
import "syscall"
import "unsafe"

func Checksum(fileName string) (pk3cs, abscs uint, err error) {
	var b []byte

	b, err = syscall.ByteSliceFromString(fileName)
	if err != nil {
		return
	}

	pk3cs = uint(C.FS_LoadPK3File((*C.char)(unsafe.Pointer(&b[0]))))
	abscs = uint(C.FS_ChecksumAbsoluteFile((*C.char)(unsafe.Pointer(&b[0]))))
	return
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage:", os.Args[0], "<pk3file>")
		os.Exit(1)
	}

	fileName := os.Args[1]
	pk3cs, abscs, err := Checksum(fileName)
	if err != nil {
		panic(err);
	}

	if pk3cs != 0 {
		fmt.Println(pk3cs, fileName, abscs)
	}
}
