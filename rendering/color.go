package main

import (
	"image/color"
	_ "log"
	"math"

	"github.com/lucasb-eyer/go-colorful"
)

func hsbToRgb(hue, saturation, brightness float64) (uint8, uint8, uint8) {
	hue /= 255.0
	saturation /= 255.0
	brightness /= 255.0

	r := 0.0
	g := 0.0
	b := 0.0
	if saturation == 0 {
		r = (brightness*255.0 + 0.5)
		g = r
		b = r
	} else {
		h := (hue - math.Floor(hue)) * 6.0
		f := h - math.Floor(h)
		p := brightness * (1.0 - saturation)
		q := brightness * (1.0 - saturation*f)
		t := brightness * (1.0 - (saturation * (1.0 - f)))
		switch int(h) {
		case 0:
			r = (brightness*255.0 + 0.5)
			g = (t*255.0 + 0.5)
			b = (p*255.0 + 0.5)
			break
		case 1:
			r = (q*255.0 + 0.5)
			g = (brightness*255.0 + 0.5)
			b = (p*255.0 + 0.5)
			break
		case 2:
			r = (p*255.0 + 0.5)
			g = (brightness*255.0 + 0.5)
			b = (t*255.0 + 0.5)
			break
		case 3:
			r = (p*255.0 + 0.5)
			g = (q*255.0 + 0.5)
			b = (brightness*255.0 + 0.5)
			break
		case 4:
			r = (t*255.0 + 0.5)
			g = (p*255.0 + 0.5)
			b = (brightness*255.0 + 0.5)
			break
		case 5:
			r = (brightness*255.0 + 0.5)
			g = (p*255.0 + 0.5)
			b = (q*255.0 + 0.5)
			break
		}
	}
	return uint8(r), uint8(g), uint8(b)
}

func combine(c1, c2 color.Color) color.Color {
	r1, g1, b1, a1 := c1.RGBA()
	r2, g2, b2, a2 := c2.RGBA()

	if false {
		cf1 := colorful.MakeColor(c1)
		cf2 := colorful.MakeColor(c2)
		return cf1.BlendRgb(cf2, 0.5)
	}

	if true {
		r0 := (r1 + r2) >> 9
		g0 := (g1 + g2) >> 9
		b0 := (b1 + b2) >> 9
		a0 := (a1 + a2) >> 9

		return color.RGBA{
			uint8(r0),
			uint8(g0),
			uint8(b0),
			uint8(a0),
		}
	}

	return c2
}
