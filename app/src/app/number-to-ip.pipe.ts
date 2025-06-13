import {Pipe, PipeTransform} from '@angular/core';

@Pipe({
  name: 'numberToIp'
})
export class NumberToIpPipe implements PipeTransform {

  transform(value: number): string | null {
    return [
      (value >> 0) & 0xFF,
      (value >> 8) & 0xFF,
      (value >> 16) & 0xFF,
      (value >> 24) & 0xFF,
    ].join('.');
  }

}
