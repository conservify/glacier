package main

import (
	"image"
	"image/color"
	"image/draw"
)

func fill(img *image.RGBA) {
	color := color.RGBA{255, 255, 255, 255}
	draw.Draw(img, img.Bounds(), &image.Uniform{color}, image.ZP, draw.Src)
}

type ColumnDrawer struct {
	image *image.RGBA
}

func NewColumnDrawer(image *image.RGBA) (cd *ColumnDrawer) {
	return &ColumnDrawer{
		image: image,
	}
}

func (cd *ColumnDrawer) DrawColumn(x, start, end int, clr *color.RGBA, fast bool) {
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
		i := cd.image.PixOffset(x, start)

		for c := start; c < end; c += 1 {
			cd.image.Pix[i+0] = clr.R
			cd.image.Pix[i+1] = clr.G
			cd.image.Pix[i+2] = clr.B
			cd.image.Pix[i+3] = clr.A

			i += cd.image.Stride
		}
	} else {
		for c := start; c < end; c += 1 {
			cd.image.Set(x, c, combine(cd.image.At(x, c), *clr))
		}
	}
}
