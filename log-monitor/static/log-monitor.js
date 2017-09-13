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

class LogDisplay extends React.Component {
    constructor() {
        super();
        this.state = {
            visible: false
        };
    }

    toggleVisible() {
        const { visible } = this.state;
        this.setState({
            visible: !visible
        });
    }

    render() {
        const { log } = this.props;
        const { visible } = this.state;

        if (!log) {
            return (<div></div>);
        }
        if (!visible) {
            return (<div className="btn btn-logs btn-sm btn-outline-secondary" onClick={() => this.toggleVisible()}>Logs</div>);
        }

        return (
                <div>
                    <div className="btn btn-logs btn-sm btn-outline-secondary" onClick={() => this.toggleVisible()}>Logs</div>
                    <ul className="log">
                    {log.map((l, k) => <li key={k}>{l}</li>)}
                    </ul>
                </div>
        );
    }
}

class Machine extends React.Component {
    renderStatus(health) {
        return (<div className="health">
                    <LastUpdatedStatus status={health.status} time={health.info.lastUpdatedAt} title="Health" />
                    <LogDisplay log={health.info.log} />
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
        return <tr key={m.mountPoint}><td>{m.mountPoint}</td><td>{size}</td><td>{available}</td><td>{m.used}%</td></tr>;
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
            <div>
                <LastUpdatedStatus status={backups.status} time={backups.info.lastUpdatedAt} title="Local Backup" />
                <LogDisplay log={backups.info.log} />
            </div>
        );
    }

    renderOffsiteBackup(backups) {
        return (
            <div>
                <LastUpdatedStatus status={backups.status} time={backups.info.lastUpdatedAt} title="Offsite Backup" />
                <LogDisplay log={backups.info.log} />
            </div>
        );
    }

    renderGeophone(geophone) {
        return (
            <div>
                <LastUpdatedStatus status={geophone.status} time={geophone.info.lastUpdatedAt} title="Geophone" />
                <LogDisplay log={geophone.info.log} />
            </div>
        );
    }

    renderUploader(uploader) {
        return (
                <div>
                <LastUpdatedStatus status={uploader.status} time={uploader.info.lastUpdatedAt} title="Uploader" />
                <LogDisplay log={uploader.info.log} />
                </div>
        );
    }

    renderResilience(resilience) {
        return (
            <div>
                <LastUpdatedStatus status={resilience.status} time={resilience.info.lastUpdatedAt} title="Resilience" />
                <LogDisplay log={resilience.info.log} />
            </div>
        );
    }

    renderCron(cron) {
        return (
            <div>
                <LastUpdatedStatus status={cron.status} time={cron.info.lastUpdatedAt} title="Cron" />
                <LogDisplay log={cron.info.log} />
            </div>
        );
    }

    renderMorningStar(morningStar) {
        return (
            <div>
                <LastUpdatedStatus status={morningStar.status} time={morningStar.info.lastUpdatedAt} title="MorningStar" />
                <LogDisplay log={morningStar.info.log} />
            </div>
        );
    }
}

class LodgeMachine extends Machine {
    render() {
        const { machine } = this.props;

        return (<div className="col-md-6">
                <h1>{machine.hostname}</h1>
                {this.renderStatus(machine.health)}
                {this.renderResilience(machine.resilience)}
                {this.renderCron(machine.cron)}
                {this.renderMorningStar(machine.morningStar)}
                {this.renderLocalBackup(machine.localBackup)}
                {this.renderOffsiteBackup(machine.offsiteBackup)}
                {this.renderMounts(machine.mounts)}
                </div>);
    }
}

class GlacierMachine extends Machine {
    render() {
        const { machine } = this.props;

        return (<div className="col-md-6">
                <h1>{machine.hostname}</h1>
                {this.renderStatus(machine.health)}
                {this.renderResilience(machine.resilience)}
                {this.renderCron(machine.cron)}
                {this.renderMorningStar(machine.morningStar)}
                {this.renderLocalBackup(machine.localBackup)}
                {this.renderOffsiteBackup(machine.offsiteBackup)}
                {this.renderMounts(machine.mounts)}
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
