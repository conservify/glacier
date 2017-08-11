/**
 *
 */

class LastUpdatedStatus extends React.Component {
    render() {
        const { status, time, title } = this.props;
        const parsed = moment(time);
        const age = moment().diff(parsed, 'minutes');

        if (status == "Good") {
            return <h3 className="time-status"><p className="bg-success">{title} <span className="age">{age} mins</span></p></h3>;
        } else if (status == "Warning") {
            return <h3 className="time-status"><p className="bg-warning">{title} <span className="age">{age} mins</span></p></h3>;
        } else if (status == "Fatal") {
            return <h3 className="time-status"><p className="bg-danger">{title} <span className="age">{age} mins</span></p></h3>;
        } else {
            return <h3 className="time-status"><p className="bg-warning">{title} <span className="age">{age} mins</span></p></h3>;
        }
    }
}

class Machine extends React.Component {
    renderStatus(health) {
        return (<div className="health">
                <LastUpdatedStatus status={health.status} time={health.info.lastUpdatedAt} title="Health" />
                    <table className="table">
                        <tbody>
                            <tr><th>Uptime</th><td>{health.info.uptime}</td></tr>
                            <tr><th>Load Average</th><td>{health.info.loadAverage}</td></tr>
                        </tbody>
                    </table>
                </div>);
    }

    humanizeBytes(bytes) {
        const GB = 1024 * 1024 * 1024;
        const MB = 1024 * 1024;
        const KB = 1024;
        if (bytes > GB) {
            return (bytes / GB).toFixed(2) + "G";
        }
        if (bytes > MB) {
            return (bytes / MB).toFixed(2) + "M";
        }
        if (bytes > KB) {
            return (bytes / KB).toFixed(2) + "k";
        }
        return bytes;
    }

    renderMount(m) {
        const size = this.humanizeBytes(m.size);
        const available = this.humanizeBytes(m.available);
        return <tr key={m.mountPoint}><td>{m.mountPoint}</td><td>{size}</td><td>{available}</td><td>%{m.used}</td></tr>;
    }

    renderMounts(mounts) {
        return (<div className="mounts">
                    <LastUpdatedStatus status={mounts.status} time={mounts.info.lastUpdatedAt} title="Mounts" />
                    <table className="table">
                        <tbody>
                        {$.map(mounts.info.mounts, (v, k) => this.renderMount(v))}
                        </tbody>
                    </table>
                </div>);
    }

    renderLocalBackup(backups) {
        return (
                <LastUpdatedStatus status={backups.status} time={backups.info.lastUpdatedAt} title="Local Backup" />
        );
    }

    renderOffsiteBackup(backups) {
        return (
                <LastUpdatedStatus status={backups.status} time={backups.info.lastUpdatedAt} title="Offsite Backup" />
        );
    }

    renderGeophone(geophone) {
        return (
                <LastUpdatedStatus status={geophone.status} time={geophone.info.lastUpdatedAt} title="Geophone" />
        );
    }

    renderUploader(uploader) {
        return (
                <LastUpdatedStatus status={uploader.status} time={uploader.lastUpdatedAt} title="Uploader" />
        );
    }
}

class LodgeMachine extends Machine {
    render() {
        const { machine } = this.props;

        return (<div className="col">
                <h1>{machine.hostname}</h1>
                {this.renderStatus(machine.health)}
                {this.renderMounts(machine.mounts)}
                {this.renderLocalBackup(machine.localBackup)}
                {this.renderOffsiteBackup(machine.offsiteBackup)}
                </div>);
    }
}

class GlacierMachine extends Machine {
    render() {
        const { machine } = this.props;

        return (<div className="col">
                <h1>{machine.hostname}</h1>
                {this.renderStatus(machine.health)}
                {this.renderMounts(machine.mounts)}
                {this.renderLocalBackup(machine.localBackup)}
                {this.renderOffsiteBackup(machine.offsiteBackup)}
                {this.renderGeophone(machine.geophone)}
                {this.renderUploader(machine.uploader)}
                </div>);
    }
}

class StatusPage extends React.Component {
    refresh() {
        return $.getJSON("status.json").then((data) => {
            return this.setState({
                lodge: data.machines['lodge'],
                glacier: data.machines['glacier'],
            });
        });
    }

    refreshAndSchedule() {
        this.refresh().then(() => {
            setTimeout(() => {
                this.refreshAndSchedule();
            }, 1000);
        });
    }

    componentWillMount() {
        this.setState( {
            machines: null
        });

        this.refreshAndSchedule();
    }

    render() {
        const { lodge, glacier } = this.state;
        if (!lodge || !glacier) {
            return <h1>Loading...</h1>;
        }
        return (
            <div className="machines row">
                <LodgeMachine machine={lodge} />
                <GlacierMachine machine={glacier} />
            </div>
        );
    }
}

var rootComponent = <StatusPage />;
ReactDOM.render(rootComponent, document.getElementById('root'));
