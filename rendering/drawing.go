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

func drawColumn(img *image.RGBA, x, start, end int, color color.RGBA) {
	if start > end {
		start, end = end, start
	}

	for c := start; c < end; c += 1 {
		img.Set(x, c, combine(img.At(x, c), color))
	}
}
