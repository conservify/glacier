package main

import (
	_ "fmt"
	"image"
	"image/color"
	"image/draw"
	"math"

	_ "github.com/pierrre/imageutil"
)

func fill(img *image.RGBA) {
	color := color.RGBA{255, 255, 255, 255}
	draw.Draw(img, img.Bounds(), &image.Uniform{color}, image.ZP, draw.Src)
}

type ColumnDrawer struct {
	image  *image.RGBA
	column int
	min    int
	max    int
}

func NewColumnDrawer(image *image.RGBA) (cd *ColumnDrawer) {
	return &ColumnDrawer{
		image: image,
	}
}

func (cd *ColumnDrawer) DrawColumn(x, start, end int, clr *color.RGBA, fast bool) {
	if true {
		if start > end {
			start, end = end, start
		}
		if start < 0 {
			start = 0
		}
		if end < 0 {
			end = 0
		}
		if start >= cd.image.Rect.Max.Y {
			start = cd.image.Rect.Max.Y - 1
		}
		if end >= cd.image.Rect.Max.Y {
			end = cd.image.Rect.Max.Y - 1
		}

		if fast {
			c1 := color.RGBAModel.Convert(*clr).(color.RGBA)

			i := cd.image.PixOffset(x, start)

			for c := start; c < end; c += 1 {
				cd.image.Pix[i+0] = c1.R
				cd.image.Pix[i+1] = c1.G
				cd.image.Pix[i+2] = c1.B
				cd.image.Pix[i+3] = c1.A

				i += cd.image.Stride
			}
		} else {
			for c := start; c < end; c += 1 {
				cd.image.Set(x, c, combine(cd.image.At(x, c), *clr))
			}
		}
	} else {
		if cd.column != x {
			cd.column = x
			cd.min = math.MaxInt32
			cd.max = math.MinInt32
		}
	}
}
