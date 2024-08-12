# P2P for file sharing

### 사용법
sending peer로 실행시
</br>Sending peer requires -n, -f, -g, and -p options

receving peer로 실행시
</br>Receiving peer requires -a and -p options

Usage: [-s] [-n max_peer] [-f file_name] [-g segment_size] [-r] [-a sender_ip:sender_port] [-p listen_port]
</br>
</br>

sending peer가 요구하는 receving peer수만큼 접속해야 프로그램 시작

</br>

### 주의사항
파일 사이즈를 너무 작게 전송할 시 throughput이 정확하게 나오지 않을 수 있음