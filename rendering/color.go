package main

import (
	"image/color"
	"log"
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

var DefaultGradientTable GradientTable

func init() {
	log.Printf("Colors are ready")
	DefaultGradientTable = CreateDefaultGradientTable()
}

func MapToColor(sample, min, max float64) color.Color {
	if false {
		hue := mapFloat(math.Abs(sample), 0, 400, 0, 255)
		r, g, b := hsbToRgb(hue, 255, 255)
		return color.RGBA{r, g, b, 255}
	}

	t := 0.0
	if sample < 0 {
		t = sample / (-8192.0 * 1.0)
	} else {
		t = sample / (8192.0 * 1.0)
	}

	return DefaultGradientTable.GetInterpolatedColorFor(t)
}

func CreateDefaultGradientTable() GradientTable {
	return GradientTable{
		{MustParseHex("#9e0142"), 0.0},
		{MustParseHex("#d53e4f"), 0.1},
		{MustParseHex("#f46d43"), 0.2},
		{MustParseHex("#fdae61"), 0.3},
		{MustParseHex("#fee090"), 0.4},
		{MustParseHex("#ffffbf"), 0.5},
		{MustParseHex("#e6f598"), 0.6},
		{MustParseHex("#abdda4"), 0.7},
		{MustParseHex("#66c2a5"), 0.8},
		{MustParseHex("#3288bd"), 0.9},
		{MustParseHex("#5e4fa2"), 1.0},
	}
}

// This table contains the "keypoints" of the colorgradient you want to generate.
// The position of each keypoint has to live in the range [0,1]
type GradientTable []struct {
	Col colorful.Color
	Pos float64
}

// This is the meat of the gradient computation. It returns a HCL-blend between
// the two colors around `t`.
// Note: It relies heavily on the fact that the gradient keypoints are sorted.
func (self GradientTable) GetInterpolatedColorFor(t float64) colorful.Color {
	for i := 0; i < len(self)-1; i++ {
		c1 := self[i]
		c2 := self[i+1]
		if c1.Pos <= t && t <= c2.Pos {
			// We are in between c1 and c2. Go blend them!
			t := (t - c1.Pos) / (c2.Pos - c1.Pos)
			return c1.Col.BlendHcl(c2.Col, t).Clamped()
		}
	}

	// Nothing found? Means we're at (or past) the last gradient keypoint.
	return self[len(self)-1].Col
}

// This is a very nice thing Golang forces you to do!
// It is necessary so that we can write out the literal of the colortable below.
func MustParseHex(s string) colorful.Color {
	c, err := colorful.Hex(s)
	if err != nil {
		panic("MustParseHex: " + err.Error())
	}
	return c
}
