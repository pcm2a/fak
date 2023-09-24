let rec _number = {
  ceil = fun n => if std.number.fract n == 0 then n else std.number.floor (n + 1),
} in

let rec _array = {
  unique = fun arr => std.array.fold_left (fun acc e =>
    if std.array.any ((==) e) acc then acc else acc @ [e]
  ) [] arr,

  enumerate = fun arr => std.array.fold_left (fun acc e => acc @ [{
    index = std.array.length acc,
    value = e
  }] ) [] arr,

  index_of = fun e arr => 
    let matches = std.array.filter (fun { index, value } => value == e) (enumerate arr) in
    (std.array.first matches).index,
  
  join = fun sep arr => std.array.fold_left (++) "" (std.array.intersperse sep arr),

  chunk = fun size arr =>
    let arr_len = std.array.length arr in
    if arr_len > size then
      std.array.generate (fun i =>
        let start = i * size in
        let end = std.number.min (start + size) arr_len in
        std.array.slice start end arr
      ) (_number.ceil (arr_len / size))
    else
      [arr],
  
  at_or = fun i default_value arr =>
    if i < std.array.length arr then
      std.array.at i arr
    else
      default_value,
} in

let Integer = std.number.Integer in
let rec _bit = {
  shift | Integer -> Integer -> Integer = fun n shift => (
    if shift >= 0 then
      n * (std.number.pow 2 shift)
    else
      std.number.truncate (n / (std.number.pow 2 (-shift)))
  ),
} in

{
  number = _number,
  array = _array,
  bit = _bit,
}