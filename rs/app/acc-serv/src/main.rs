//
// weproxy@foxmail.com 2022/10/22
//

mod api;
mod conf;
mod core;
mod proto;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    librs::logx::init_default()?;

    let threads = match num_cpus::get() {
        0 | 1 => 2,
        n => n,
    };
    // info!("[dbg] {}:{} threads = {}", file!(), line!(),threads);
    info!(target: "aaa","threads = {}", threads);

    // let rt = tokio::runtime::Runtime::new()?;
    let rt = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(threads)
        .thread_stack_size(1024 * 1024)
        .on_thread_start(|| {
            info!("[tokio] thread started");
        })
        .on_thread_stop(|| {
            info!("[tokio] thread stopped");
        })
        .enable_all()
        .build()
        .map_err(|e| anyhow::anyhow!("create tokio::runtime fail: {}", e))?;

    rt.spawn(async move {
        if let Err(e) = core::App::new().run().await {
            info!("[app] run: {}", e);
            // std::process::exit(-1);
        }
    });

    rt.block_on(async {
        let _ = tokio::signal::ctrl_c().await;
        info!("Ctrl+C");
    });

    Ok(())
}
