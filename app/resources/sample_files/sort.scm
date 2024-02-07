; sort : list-of-numbers => sorted-list-of-numbers
; 😳👨‍👩‍👦👩‍👩‍👧‍👦🇺🇸🏴‍☠️
; 11111 22222 33333 44444 55555 66666 77777 88888 99999 1010101010 1111111111 1212121212 1313131313 1414141414 1515151515 1616161616 1717171717 1818181818 1919191919 2020202020 2121212121 2222222222 2323232323 2424242424 2525252525 2626262626 2727272727 2828282828 2929292929 3030303030
; 䲜䨻ကခဂဃငစဆဇဈဉ
(define (sort nums)
  (local [(define (insert-in-order newnum sorted-nums)
            (cond [(empty? sorted-nums) (list newnum)]
                  [(cons? sorted-nums)
                   (cond [(<= newnum (first sorted-nums))
                          (cons newnum sorted-nums)]
                         [else
                          (cons (first sorted-nums)
                                (insert-in-order newnum (rest sorted-nums)))])]))
          (define (helper nums)
            (cond [(empty? nums) empty]
                  [(cons? nums)
                   (insert-in-order (first nums)
                                    (helper (rest nums)))]))]
         (helper nums)))
"Examples of sort:"
(sort empty) "should be" empty
(sort (list 5)) "sb" (list 5)
(sort (list 5 2)) "sb" (list 2 5)
(sort (list 2 5)) "sb" (list 2 5)
(sort (list 6 9 5 2 7 3 5)) "should be" (list 2 3 5 5 6 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)

; sort : list-of-numbers => sorted-list-of-numbers
(define (sort nums)
  (local [(define (insert-in-order newnum sorted-nums)
            (cond [(empty? sorted-nums) (list newnum)]
                  [(cons? sorted-nums)
                   (cond [(<= newnum (first sorted-nums))
                          (cons newnum sorted-nums)]
                         [else
                          (cons (first sorted-nums)
                                (insert-in-order newnum (rest sorted-nums)))])]))
          (define (helper nums)
            (cond [(empty? nums) empty]
                  [(cons? nums)
                   (insert-in-order (first nums)
                                    (helper (rest nums)))]))]
         (helper nums)))
"Examples of sort:"
(sort empty) "should be" empty
(sort (list 5)) "sb" (list 5)
(sort (list 5 2)) "sb" (list 2 5)
(sort (list 2 5)) "sb" (list 2 5)
(sort (list 6 9 5 2 7 3 5)) "should be" (list 2 3 5 5 6 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)
(sort (list 1 9 5 2 7 3 5)) "should be" (list 1 2 3 5 5 7 9)
