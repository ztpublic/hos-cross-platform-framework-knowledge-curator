@file:OptIn(kotlin.experimental.ExperimentalNativeApi::class, kotlinx.cinterop.ExperimentalForeignApi::class)
package org.jetbrains.kotlinx.io.example
import kotlinx.cinterop.*
private fun textBuffer(text: String): CPointer<ByteVar> { val bytes=text.encodeToByteArray(); val b=nativeHeap.allocArray<ByteVar>(bytes.size+1); bytes.forEachIndexed { i,v -> b[i]=v }; b[bytes.size]=0; return b }
private fun quote(value: String): String = buildString { append('"'); value.forEach { when(it) { '\\'->append("\\\\"); '"'->append("\\\""); '\n'->append("\\n"); '\r'->append("\\r"); '\t'->append("\\t"); else->append(it) } }; append('"') }
private inline fun response(block: () -> String): CPointer<ByteVar> = try { textBuffer(block()) } catch (e: Throwable) { textBuffer("{\"error\":${quote(e.message ?: "Native kotlinx-io error")}}") }
@CName("IoCatalog") public fun catalogNative(): CPointer<ByteVar> = response { IoExamples.catalog().joinToString(prefix="[", postfix="]") { it.toJson() } }
@CName("IoGet") public fun ioNative(index: Int): CPointer<ByteVar> = response { IoExamples.get(index).toJson() }
@CName("IoCurrent") public fun currentNative(): CPointer<ByteVar> = response { IoExamples.random().toJson() }
@CName("IoRunChecks") public fun checksNative(): CPointer<ByteVar> = response { val checks=IoExamples.runChecks(); "{\"passed\":true,\"checks\":[${checks.joinToString { quote(it) }}]}" }
@CName("IoFree") public fun freeNative(pointer: CPointer<ByteVar>?) { if(pointer != null) nativeHeap.free(pointer.rawValue) }
private fun IoExample.toJson() = "{\"id\":${quote(id)},\"value\":${quote(value)},\"byteCount\":$byteCount,\"roundTrip\":$roundTrip}"
