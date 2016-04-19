package main

import "testing"

func TestBasic(t *testing.T) {
	expectedPk3Checksum := uint(1505803646)
	expectedAbsChecksum := uint(2236316694)

	pk3cs, abscs, err := Checksum("tests/test.pk3")
	if err != nil {
		t.Fatal(err)
	}
	if pk3cs != expectedPk3Checksum {
		t.Fatal("%#v != %#v", pk3cs, expectedPk3Checksum)
	}
	if abscs != expectedAbsChecksum {
		t.Fatal("%#v != %#v", abscs, expectedAbsChecksum)
	}
}

func TestInvalid(t *testing.T) {
	expectedPk3Checksum := uint(0)
	expectedAbsChecksum := uint(2740404939)

	pk3cs, abscs, err := Checksum("tests/invalid.pk3")
	if err != nil {
		t.Fatal(err)
	}
	if pk3cs != expectedPk3Checksum {
		t.Fatal("%#v != %#v", pk3cs, expectedPk3Checksum)
	}
	if abscs != expectedAbsChecksum {
		t.Fatal("%#v != %#v", abscs, expectedAbsChecksum)
	}
}
