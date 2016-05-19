package main

//#include "md5.h"
//#include "files.h"
import "C"
import (
	"bytes"
	"fmt"
	"os"
	"sync"
	"syscall"
	"unsafe"
)

func ChecksumFile(fileName string) (pk3cs, abscs uint, err error) {
	var b []byte
	var errbuf = make([]byte, 1024)

	b, err = syscall.ByteSliceFromString(fileName)
	if err != nil {
		return
	}

	pk3cs = uint(C.FS_LoadPK3File((*C.char)(unsafe.Pointer(&b[0])), (*C.char)(unsafe.Pointer(&errbuf[0])), C.uint(len(errbuf))))
	abscs = uint(C.FS_ChecksumAbsoluteFile((*C.char)(unsafe.Pointer(&b[0]))))

	if pk3cs == 0 {
		b := bytes.IndexByte(errbuf, 0x0)
		if b == -1 {
			err = fmt.Errorf("%s", string("Unknown error"))
		} else {
			err = fmt.Errorf("%s", string(errbuf[:b]))
		}
	}

	return
}

func ChecksumFiles(fileNames []string) (pk3cs, abscs []uint, err []error) {
	pk3cs = make([]uint, len(fileNames))
	abscs = make([]uint, len(fileNames))
	err = make([]error, len(fileNames))

	var wg sync.WaitGroup

	for i, _ := range fileNames {
		wg.Add(1)
		go func(index int) {
			pk3cs[index], abscs[index], err[index] = ChecksumFile(fileNames[index])
			wg.Done()
		}(i)
	}

	wg.Wait()

	return
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage:", os.Args[0], "<pk3file1> [pk3file2]")
		os.Exit(1)
	}

	fileNames := os.Args[1:]
	pk3cs, abscs, err := ChecksumFiles(fileNames)

	for i, _ := range fileNames {
		if pk3cs[i] != 0 {
			fmt.Println(pk3cs[i], fileNames[i], abscs[i])
		} else {
			fmt.Println("//", err[i])
		}
	}
}
