$host_addr = "127.0.0.1"
$host_port = 6454

$udpClient = [System.Net.Sockets.UdpClient]::new()
$udpClient.Connect($host_addr, $host_port)

try {
	Write-Host "Testing multiple Art-Net packet types (DMX, Poll, Sync, Address, DiagData, etc.)..."
	Write-Host "Target: ${host_addr}:${host_port}"
	Write-Host ""
	Write-Host ""
	Write-Host ""

	$rng = [System.Random]::new()
	$cycle = 0
	$pktCount = 0
	$lastMiscPkt = $null

	$statusRow = [Console]::CursorTop - 3

	function Update-Status {
		$saved = [Console]::CursorTop

		[Console]::SetCursorPosition(0, $statusRow)
		Write-Host ("  Cycle         : {0,-8}" -f $cycle) -NoNewline

		[Console]::SetCursorPosition(0, $statusRow + 1)
		Write-Host ("  Total Pkts    : {0,-8}" -f $pktCount) -NoNewline

		[Console]::SetCursorPosition(0, $statusRow + 2)
		Write-Host ("  Last Misc Pkt : {0,-20}" -f $lastMiscPkt) -NoNewline

		[Console]::SetCursorPosition(0, $saved)
	}

	# Cached headers
	$sig = [System.Text.Encoding]::ASCII.GetBytes("Art-Net") + [byte[]](0x00)
	$protVer = [byte[]](0x00, 0x0E)

	while ($true) {
		$cycle++

		for ($i = 0; $i -lt 5; $i++) {
			# OpCode ArtDMX = 0x5000, little-endian on wire
			$opCode = [byte[]](0x00, 0x50)

			# Sequence, Physical
			$sequence = [byte[]]([byte](($pktCount + 1) -band 0xFF))
			$physical = [byte[]](0x00)

			# Universe 0, little-endian
			$universe = [byte[]](0x00, 0x00)

			# 512 random DMX slots
			$dmxData = [byte[]]::new(512)
			$rng.NextBytes($dmxData)

			# Length 512 = 0x0200, big-endian
			$length = [byte[]](0x02, 0x00)

			$packet = $sig + $opCode + $protVer + $sequence + $physical + $universe + $length + $dmxData

			$udpClient.Send($packet, $packet.Length) | Out-Null
			$pktCount++

			Start-Sleep -Milliseconds 10
		}

		# ArtSync (OpCode 0x5200)
		$syncPacket = $sig + [byte[]](0x00, 0x52) + $protVer + [byte[]](0x00, 0x00)
		$udpClient.Send($syncPacket, $syncPacket.Length) | Out-Null
		$pktCount++

		# Interleave other packets
		if ($cycle % 5 -eq 0) {
			# ArtPoll (OpCode 0x2000)
			$pollPacket = $sig + [byte[]](0x00, 0x20) + $protVer + [byte[]](0x00, 0x00)
			$udpClient.Send($pollPacket, $pollPacket.Length) | Out-Null
			$pktCount++
			$lastMiscPkt = "ArtPoll"
		}
		elseif ($cycle % 5 -eq 1) {
			# ArtDiagData (OpCode 0x2300)
			$diagText = [System.Text.Encoding]::ASCII.GetBytes("Test Diagnostic Message")
			$diagPacket = $sig + [byte[]](0x00, 0x23) + $protVer + [byte[]](0x00, 0x10, 0x00, 0x00) + [byte[]](0x00, $diagText.Length) + $diagText
			$udpClient.Send($diagPacket, $diagPacket.Length) | Out-Null
			$pktCount++
			$lastMiscPkt = "ArtDiagData"
		}
		elseif ($cycle % 5 -eq 2) {
			# ArtAddress (OpCode 0x6000)
			$filler = [byte[]]::new(107)
			$addrPacket = $sig + [byte[]](0x00, 0x60) + $protVer + [byte[]](0x00, 0x00) + $filler
			$udpClient.Send($addrPacket, $addrPacket.Length) | Out-Null
			$pktCount++
			$lastMiscPkt = "ArtAddress"
		}
		elseif ($cycle % 5 -eq 3) {
			# ArtTimeCode (OpCode 0x9700)
			$tcFiller = [byte[]]::new(15)
			$tcPacket = $sig + [byte[]](0x00, 0x97) + $protVer + [byte[]](0x00, 0x00) + $tcFiller
			$udpClient.Send($tcPacket, $tcPacket.Length) | Out-Null
			$pktCount++
			$lastMiscPkt = "ArtTimeCode"
		}
		elseif ($cycle % 5 -eq 4) {
			# ArtTrigger (OpCode 0x9900)
			$trigFiller = [byte[]]::new(512)
			$triggerPacket = $sig + [byte[]](0x00, 0x99) + $protVer + [byte[]](0x00, 0x00) + $trigFiller
			$udpClient.Send($triggerPacket, $triggerPacket.Length) | Out-Null
			$pktCount++
			$lastMiscPkt = "ArtTrigger"
		}

		Update-Status
	}
}
finally {
	Write-Host ""
	Write-Host ""
	Write-Host "Closing UDP client..."
	$udpClient.Close()
	Write-Host "Done."
}