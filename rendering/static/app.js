const cache = {};

let previous = null;

function shouldRender(data) {
    const rendering = [];
    if (previous == null || previous.NumberOfFiles != data.NumberOfFiles || previous.Start != data.Start || previous.End != data.End) {
        previous = data;

        rendering.push(data.AvailableHours[data.AvailableHours.length - 2]);
        rendering.push(data.Hour);
    }
    return rendering;
}

function refresh() {
    $.getJSON('/status.json', (data) => {
        const r = shouldRender(data);
        if (r.length == 0) {
            setTimeout(() => {
                refresh();
            }, 10 * 1000);
        }
        else {
            for (let i = 0; i < r.length; i++) {
                const hour = r[i];
                const img = cache[hour] = new Image();
                img.src = "/rendering.png?hour=" + hour;
                img.style.display = "none";

                img.onload = () => {
                    console.log("Load");
                    $(img).attr("data-hour", hour).show();
                    if (i == r.length - 1) {
                        refresh();
                    }
                };

                $('#app').prepend(img);
            }
        }
    });
}

refresh();
