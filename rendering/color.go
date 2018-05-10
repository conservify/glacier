package main

import (
	"image/color"
	"math"
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
	r, g, b, a := c1.RGBA()
	r2, g2, b2, a2 := c2.RGBA()

	return color.RGBA{
		uint8((r + r2) >> 9), // div by 2 followed by ">> 8"  is ">> 9"
		uint8((g + g2) >> 9),
		uint8((b + b2) >> 9),
		uint8((a + a2) >> 9),
	}
}
