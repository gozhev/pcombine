pkgname=pcombine
pkgver=1.0_alpha.1
pkgrel=1
pkgdesc="A program to simultaneously run multiple programs and kill all of them if one dies"
arch=(any)
url="https://github.com/gozhev/pcombine"
license=("GPL")

source=("${pkgname}-${pkgver//_/-}.tar.gz"::"https://github.com/gozhev/pcombine/archive/refs/tags/v${pkgver//_/-}.tar.gz")
md5sums=('9463daa27a1263d1e1c3d7d6be664a75')

build() {
	cd "${pkgname}-${pkgver//_/-}"
	make
}

package() {
	cd "${pkgname}-${pkgver//_/-}"
	install -D -m755 "${pkgname}" "${pkgdir}/usr/bin/${pkgname}"
}
