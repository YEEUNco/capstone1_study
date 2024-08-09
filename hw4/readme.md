# 자동 완성 기능 기반 search engine 구현

### 사용법
search word: 옆에 단어를 검색한다

enter키 사용시 string검색 완료

그 후 Enter q to quit: 이 문장이 나왔을 때 q 또는 Q 누르면 search engine 종료

그 외의 키를 눌렀을 시 다시 reset된 search word를 시작할 수 있다.



### 주의사항
- delete키 지원 안함!! 잘못 눌렀으면 enter키 실행 후 처음부터 다시 검색하세요!

- trie.c 파일 안 set_data함수에서 txt 변경 가능

    단, string이랑 frequency사이에 tab으로 구분 되어있어야 parsing 가능