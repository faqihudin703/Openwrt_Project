from ast import Return


def tampilkanMenu():
    print("Pilih operasi yang ingin Anda lakukan:\n1. Penjumlahan\n2. Pengurangan\n3. Perkalian\n4. Pembagian\n5. Keluar\n")

def hitung(pilihan):
    a = float(input("Masukkan angka pertama \t: "))
    b = float(input("Masukkan angka kedua \t: "))
    if pilihan == 1:
        hasil = a + b
        print("Hasil penjumlahan adalah", hasil)
    elif pilihan == 2:
        hasil = a - b
        print("Hasil pengurangan adalah", hasil)
    elif pilihan == 3:
        hasil = a * b
        print("Hasil perkalian adalah", hasil)
    elif pilihan == 4:
        if b == 0:
            print("Tidak bisa membagi dengan nol")
        else:
            hasil = a / b
            print("Hasil pembagian adalah", hasil)
    elif pilihan == 5:
        print("Terima kasih telah menggunakan kalkulator ini")
    else:
        print("Pilihan tidak valid")

while True:
    tampilkanMenu()
    pilihan = int(input("Masukkan pilihan Anda \t: "))
    hitung(pilihan)
    print()