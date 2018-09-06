const cache = {};

let previous = null;

function hourChanged(previous, current) {
    return previous.Hour != current.Hour || previous.NumberOfFiles != current.NumberOfFiles;
}

function shouldRender(data) {
    const rendering = [];
    if (previous == null) {
        rendering.push(data.PreviousHour.Hour);
        rendering.push(data.CurrentHour.Hour);
    }
    else {
        if (hourChanged(previous.PreviousHour, data.PreviousHour)) {
            rendering.push(data.PreviousHour.Hour);
        }
        if (hourChanged(previous.CurrentHour, data.CurrentHour)) {
            rendering.push(data.CurrentHour.Hour);
        }
    }

    previous = data;

    return rendering;
}

function refresh() {
    $.getJSON('/status.json', (data) => {
        const r = shouldRender(data);
        if (r.length == 0) {
            setTimeout(() => {
                refresh();
            }, 30 * 1000);
        }
        else {
            for (let i = 0; i < r.length; i++) {
                const hour = r[i];
                const existing = cache[hour];

                console.log(hour, "Loading", existing);

                const img = cache[hour] = new Image();
                img.src = "/rendering.png?hour=" + hour;
                img.style.display = "none";

                img.onload = () => {
                    if (existing) {
                        $(existing).replaceWith(img);
                        console.log(hour, "Done, replaced");
                    }
                    else {
                        console.log(hour, "Done, first");
                    }

                    $(img).show();

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
