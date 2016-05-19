package main

import "testing"

const (
	expectedTestPk3Checksum uint = 1505803646
	expectedTestAbsChecksum = 2236316694

	expectedInvalidPk3Checksum = 0
	expectedInvalidAbsChecksum = 2740404939
)

func TestBasic(t *testing.T) {
	pk3cs, abscs, err := ChecksumFile("tests/test.pk3")
	if pk3cs != expectedTestPk3Checksum {
		if err != nil {
			t.Fatal(err)
		}
		t.Fatal("%#v != %#v %s", pk3cs, expectedTestPk3Checksum)
	}
	if abscs != expectedTestAbsChecksum {
		t.Fatal("%#v != %#v", abscs, expectedTestAbsChecksum)
	}
}

func TestInvalid(t *testing.T) {
	pk3cs, abscs, err := ChecksumFile("tests/invalid.pk3")
	if pk3cs != expectedInvalidPk3Checksum {
		if err != nil {
			t.Fatal(err)
		}
		t.Fatal("%#v != %#v %s", pk3cs, expectedInvalidPk3Checksum)
	}
	if abscs != expectedInvalidAbsChecksum {
		t.Fatal("%#v != %#v", abscs, expectedInvalidAbsChecksum)
	}
}

func TestMulti(t *testing.T) {
	pk3cs1, abscs1, err1 := ChecksumFiles([]string{"tests/test.pk3", "tests/invalid.pk3"})
	pk3cs2, abscs2, _ := ChecksumFiles([]string{"tests/invalid.pk3", "tests/test.pk3"})

	if pk3cs1[0] != pk3cs2[1] {
		t.Fatal("%#v != %#v %s", pk3cs1[0], pk3cs2[1])
	}
	if abscs1[0] != abscs2[1] {
		t.Fatal("%#v != %#v %s", pk3cs1[0], pk3cs2[1])
	}

	if pk3cs1[0] != expectedTestPk3Checksum {
		if err1[0] != nil {
			t.Fatal(err1[0])
		}
		t.Fatal("%#v != %#v %s", pk3cs1[0], expectedTestPk3Checksum)
	}
	if abscs1[0] != expectedTestAbsChecksum {
		t.Fatal("%#v != %#v", abscs1[0], expectedTestAbsChecksum)
	}

	if pk3cs1[1] != expectedInvalidPk3Checksum {
		if err1[1] != nil {
			t.Fatal(err1[1])
		}
		t.Fatal("%#v != %#v %s", pk3cs1[1], expectedInvalidPk3Checksum)
	}
	if abscs1[1] != expectedInvalidAbsChecksum {
		t.Fatal("%#v != %#v", abscs1[1], expectedInvalidAbsChecksum)
	}
}
