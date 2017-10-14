package icenet

import chisel3._
import chisel3.util._
import testchipip.{StreamChannel, SeqQueue}
import IceNetConsts._

class LengthEncoder extends Module {
  val io = IO(new Bundle {
    val stream = Flipped(Decoupled(new StreamChannel(NET_IF_WIDTH)))
    val lencode = Decoupled(UInt(NET_IF_WIDTH.W))
  })

  val len = RegInit(0.U(ETH_PAD_BITS.W))
  val idx = RegInit(0.U(ETH_PAD_BITS.W))

  val buffer = Mem(ETH_MAX_BYTES / NET_IF_BYTES, UInt(NET_IF_WIDTH.W))
  val s_buffer :: s_send :: Nil = Enum(2)
  val state = RegInit(s_buffer)

  when (io.stream.fire()) {
    buffer(len) := io.stream.bits.data
    len := len + 1.U
    when (io.stream.bits.last) {
      idx := 0.U
      state := s_send
    }
  }

  when (io.lencode.fire()) {
    idx := idx + 1.U
    when (idx === len - 1.U) {
      len := 0.U
      state := s_buffer
    }
  }

  val senddata = buffer(idx)

  io.lencode.valid := state === s_send
  io.lencode.bits := Mux(idx === 0.U,
    senddata & ~0xffff.U(NET_IF_WIDTH.W) | len, senddata)
  io.stream.ready := state === s_buffer
}

object LengthEncoder {
  def apply(stream: DecoupledIO[StreamChannel]): DecoupledIO[UInt] = {
    val encoder = Module(new LengthEncoder)
    encoder.io.stream <> SeqQueue(stream, ETH_MAX_BYTES / NET_IF_BYTES)
    encoder.io.lencode
  }
}

class LengthDecoder extends Module {
  val io = IO(new Bundle {
    val lencode = Flipped(Decoupled(UInt(NET_IF_WIDTH.W)))
    val stream = Decoupled(new StreamChannel(NET_IF_WIDTH))
  })

  val len = RegInit(0.U(ETH_PAD_BITS.W))

  when (io.lencode.fire()) {
    when (len === 0.U) {
      len := io.lencode.bits(ETH_PAD_BITS - 1, 0) - 1.U
    } .otherwise {
      len := len - 1.U
    }
  }

  io.stream.valid := io.lencode.valid
  io.stream.bits.data := Mux(len === 0.U,
    io.lencode.bits & ~0xffff.U(NET_IF_WIDTH.W), io.lencode.bits)
  io.stream.bits.keep := NET_FULL_KEEP
  io.stream.bits.last := len === 1.U
  io.lencode.ready := io.stream.ready
}

object LengthDecoder {
  def apply(lencode: DecoupledIO[UInt]): DecoupledIO[StreamChannel] = {
    val decoder = Module(new LengthDecoder)
    decoder.io.lencode <> lencode
    decoder.io.stream
  }
}
