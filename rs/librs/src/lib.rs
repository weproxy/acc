//
// weproxy@foxmail.com 2022/10/22
//

pub mod fx;
pub mod gx;
pub mod logx;
pub mod nx;

pub type Runner = futures::future::BoxFuture<'static, ()>;
pub type RuntimeID = u16;

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }
}
